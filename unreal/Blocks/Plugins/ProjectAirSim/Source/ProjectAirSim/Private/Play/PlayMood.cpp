// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#include "Play/PlayMood.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Play/PlayLog.h"

namespace {
constexpr float kUnset = FPlayMoodEntry::kUnset;
bool IsSet(float V) { return V > kUnset + 1.f; }
bool IsSet(const FLinearColor& C) { return C.A >= 0.f; }

// The level's sun: the atmosphere-sun-flagged light if there is one, else the brightest.
ADirectionalLight* PickSun(UWorld& World) {
  ADirectionalLight* Best = nullptr;
  for (TActorIterator<ADirectionalLight> It(&World); It; ++It) {
    if (It->IsActorBeingDestroyed()) continue;
    UDirectionalLightComponent* C = Cast<UDirectionalLightComponent>(It->GetLightComponent());
    if (!C) continue;
    if (C->IsUsedAsAtmosphereSunLight()) return *It;
    if (!Best || C->Intensity > Best->GetLightComponent()->Intensity) Best = *It;
  }
  return Best;
}

// "A=1.5;B=(R=1,G=0.5,B=0,A=1)" -> pairs; a malformed entry is fatal.
TArray<TPair<FString, FString>> ParseParams(const FString& Spec) {
  TArray<TPair<FString, FString>> Out;
  TArray<FString> Items;
  Spec.ParseIntoArray(Items, TEXT(";"), true);
  for (const FString& Item : Items) {
    FString Key, Value;
    if (!Item.Split(TEXT("="), &Key, &Value)) {
      UE_LOG(LogPlay, Fatal, TEXT("Mood Params entry '%s' is not Name=Value"), *Item);
    }
    Out.Emplace(Key.TrimStartAndEnd(), Value.TrimStartAndEnd());
  }
  return Out;
}
}  // namespace

bool FPlayMoodDirector::ActorsStale() const {
  if (Settings.MoodParamCollection.IsEmpty() == false && !ParamInstance.IsValid()) return true;
  return !Sun.IsValid() || !Sky.IsValid() || !Fog.IsValid() || !Grade.IsValid();
}

bool FPlayMoodDirector::FindActors(UWorld& World) {
  bool bChanged = false;
  if (!Sun.IsValid()) {
    if (ADirectionalLight* Found = PickSun(World)) {
      Sun = Found;
      UDirectionalLightComponent* C = Cast<UDirectionalLightComponent>(Found->GetLightComponent());
      SunBase = {Found->GetActorRotation(), C->Intensity, C->GetLightColor(), C->bUseTemperature, C->Temperature,
                 C->LightSourceAngle};
      UE_LOG(LogPlay, Log, TEXT("Mood: sun %s base elevation %.1f yaw %.1f intensity %.1f temp %d/%.0f angle %.2f"),
             *Found->GetName(), -SunBase.Rotation.Pitch, SunBase.Rotation.Yaw, SunBase.Intensity,
             SunBase.bUseTemperature, SunBase.Temperature, SunBase.SourceAngle);
      bChanged = true;
    }
  }
  if (!Sky.IsValid()) {
    for (TActorIterator<ASkyLight> It(&World); It; ++It) {
      if (It->IsActorBeingDestroyed()) continue;
      Sky = *It;
      USkyLightComponent* C = It->GetLightComponent();
      SkyBase = {C->Intensity, C->GetLightColor()};
      UE_LOG(LogPlay, Log, TEXT("Mood: sky light %s base intensity %.2f"), *It->GetName(), SkyBase.Intensity);
      bChanged = true;
      break;
    }
  }
  if (!Fog.IsValid()) {
    for (TActorIterator<AExponentialHeightFog> It(&World); It; ++It) {
      if (It->IsActorBeingDestroyed()) continue;
      Fog = *It;
      UExponentialHeightFogComponent* C = It->GetComponent();
      FogBase = {C->FogDensity, C->FogHeightFalloff, C->FogInscatteringLuminance};
      UE_LOG(LogPlay, Log, TEXT("Mood: height fog %s base density %.4f falloff %.3f"), *It->GetName(),
             FogBase.Density, FogBase.HeightFalloff);
      bChanged = true;
      break;
    }
  }
  if (!Grade.IsValid()) {
    FActorSpawnParameters Params;
    Params.Name = TEXT("PlayMoodGrade");
    Params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    APostProcessVolume* Volume = World.SpawnActor<APostProcessVolume>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
    check(Volume);
    Volume->bUnbound = true;
    Volume->BlendWeight = 1.f;
    Volume->Priority = 1000.f;
    Grade = Volume;
    bChanged = true;
  }
  if (!Settings.MoodParamCollection.IsEmpty()) {
    UMaterialParameterCollection* Collection =
        LoadObject<UMaterialParameterCollection>(nullptr, *Settings.MoodParamCollection);
    if (!Collection) {
      UE_LOG(LogPlay, Fatal, TEXT("MoodParamCollection '%s' could not be loaded"), *Settings.MoodParamCollection);
    }
    UMaterialParameterCollectionInstance* Instance = World.GetParameterCollectionInstance(Collection);
    if (Instance != ParamInstance.Get()) {
      ParamInstance = Instance;
      ScalarBase.Reset();
      VectorBase.Reset();
      for (const FCollectionScalarParameter& P : Collection->ScalarParameters) {
        float V = P.DefaultValue;
        Instance->GetScalarParameterValue(P.ParameterName, V);
        ScalarBase.Add(P.ParameterName, V);
        UE_LOG(LogPlay, Log, TEXT("Mood: parameter %s = %.4f"), *P.ParameterName.ToString(), V);
      }
      for (const FCollectionVectorParameter& P : Collection->VectorParameters) {
        FLinearColor V = P.DefaultValue;
        Instance->GetVectorParameterValue(P.ParameterName, V);
        VectorBase.Add(P.ParameterName, V);
        UE_LOG(LogPlay, Log, TEXT("Mood: parameter %s = %s"), *P.ParameterName.ToString(), *V.ToString());
      }
      bChanged = true;
    }
  }
  return bChanged;
}

void FPlayMoodDirector::Apply(UWorld& World, int32 Index) {
  check(Settings.Moods.IsValidIndex(Index));
  FindActors(World);
  CurrentIndex = Index;
  const FPlayMoodEntry& Mood = Settings.Moods[Index];
  ApplyToActors(World, Mood);
  ApplyGrade(World, Mood);
  ApplyParams(World, Mood);
  TArray<FString> Commands;
  Mood.Commands.ParseIntoArray(Commands, TEXT(";"), true);
  for (const FString& Command : Commands) {
    if (!GEngine->Exec(&World, *Command.TrimStartAndEnd())) {
      UE_LOG(LogPlay, Error, TEXT("Mood '%s': command '%s' was not handled"), *Mood.Name, *Command);
    }
  }
  UE_LOG(LogPlay, Log, TEXT("Mood applied: %s"), *Mood.Name);
}

void FPlayMoodDirector::Tick(UWorld& World) {
  if (CurrentIndex == INDEX_NONE) return;
  const double Now = FPlatformTime::Seconds();
  if (Now < NextCheck) return;
  NextCheck = Now + 1.0;
  if (!ActorsStale()) return;
  UE_LOG(LogPlay, Log, TEXT("Mood: a scene load replaced the level's lights or parameters; applying '%s' again"),
         *Settings.Moods[CurrentIndex].Name);
  Apply(World, CurrentIndex);
}

void FPlayMoodDirector::ApplyToActors(UWorld&, const FPlayMoodEntry& M) {
  if (ADirectionalLight* S = Sun.Get()) {
    UDirectionalLightComponent* C = Cast<UDirectionalLightComponent>(S->GetLightComponent());
    FRotator Rotation = SunBase.Rotation;
    if (IsSet(M.SunElevationDeg)) Rotation.Pitch = -M.SunElevationDeg;
    if (IsSet(M.SunYawDeg)) Rotation.Yaw = M.SunYawDeg;
    S->SetActorRotation(Rotation);
    C->SetIntensity(IsSet(M.SunIntensity) ? M.SunIntensity : SunBase.Intensity);
    C->SetLightColor(IsSet(M.SunTint) ? M.SunTint : SunBase.Color, false);
    C->SetUseTemperature(IsSet(M.SunTemperatureK) ? true : SunBase.bUseTemperature);
    C->SetTemperature(IsSet(M.SunTemperatureK) ? M.SunTemperatureK : SunBase.Temperature);
    C->SetLightSourceAngle(IsSet(M.SunSourceAngleDeg) ? M.SunSourceAngleDeg : SunBase.SourceAngle);
  }
  if (ASkyLight* K = Sky.Get()) {
    USkyLightComponent* C = K->GetLightComponent();
    C->SetIntensity(IsSet(M.SkyIntensity) ? M.SkyIntensity : SkyBase.Intensity);
    C->SetLightColor(IsSet(M.SkyTint) ? M.SkyTint : SkyBase.Color);
  }
  if (AExponentialHeightFog* F = Fog.Get()) {
    UExponentialHeightFogComponent* C = F->GetComponent();
    C->SetFogDensity(IsSet(M.FogDensity) ? M.FogDensity : FogBase.Density);
    C->SetFogHeightFalloff(IsSet(M.FogHeightFalloff) ? M.FogHeightFalloff : FogBase.HeightFalloff);
    C->SetFogInscatteringColor(IsSet(M.FogColor) ? M.FogColor : FogBase.Inscattering);
  }
}

void FPlayMoodDirector::ApplyGrade(UWorld&, const FPlayMoodEntry& M) {
  APostProcessVolume* Volume = Grade.Get();
  if (!Volume) return;
  FPostProcessSettings S;
  if (IsSet(M.ExposureBiasEv)) { S.bOverride_AutoExposureBias = true; S.AutoExposureBias = M.ExposureBiasEv; }
  if (IsSet(M.ExposureMinEv)) { S.bOverride_AutoExposureMinBrightness = true; S.AutoExposureMinBrightness = M.ExposureMinEv; }
  if (IsSet(M.ExposureMaxEv)) { S.bOverride_AutoExposureMaxBrightness = true; S.AutoExposureMaxBrightness = M.ExposureMaxEv; }
  if (IsSet(M.Saturation)) {
    S.bOverride_ColorSaturation = true;
    S.ColorSaturation = FVector4(M.Saturation, M.Saturation, M.Saturation, 1.f);
  }
  if (IsSet(M.Contrast)) {
    S.bOverride_ColorContrast = true;
    S.ColorContrast = FVector4(M.Contrast, M.Contrast, M.Contrast, 1.f);
  }
  if (IsSet(M.ShadowTint)) {
    S.bOverride_ColorGainShadows = true;
    S.ColorGainShadows = FVector4(M.ShadowTint.R, M.ShadowTint.G, M.ShadowTint.B, 1.f);
  }
  if (IsSet(M.HighlightTint)) {
    S.bOverride_ColorGainHighlights = true;
    S.ColorGainHighlights = FVector4(M.HighlightTint.R, M.HighlightTint.G, M.HighlightTint.B, 1.f);
  }
  Volume->Settings = S;
}

void FPlayMoodDirector::ApplyParams(UWorld&, const FPlayMoodEntry& M) {
  UMaterialParameterCollectionInstance* Instance = ParamInstance.Get();
  if (!Instance) return;
  for (const TPair<FName, float>& Base : ScalarBase) Instance->SetScalarParameterValue(Base.Key, Base.Value);
  for (const TPair<FName, FLinearColor>& Base : VectorBase) Instance->SetVectorParameterValue(Base.Key, Base.Value);
  for (const TPair<FString, FString>& Pair : ParseParams(M.Params)) {
    const FName Name(*Pair.Key);
    if (Pair.Value.StartsWith(TEXT("("))) {
      FLinearColor Color;
      if (!Color.InitFromString(Pair.Value)) {
        UE_LOG(LogPlay, Fatal, TEXT("Mood '%s': Params colour '%s' is not (R=,G=,B=,A=)"), *M.Name, *Pair.Value);
      }
      if (!VectorBase.Contains(Name)) UE_LOG(LogPlay, Fatal, TEXT("Mood '%s': no vector parameter '%s' in the collection"), *M.Name, *Pair.Key);
      Instance->SetVectorParameterValue(Name, Color);
    } else {
      if (!ScalarBase.Contains(Name)) UE_LOG(LogPlay, Fatal, TEXT("Mood '%s': no scalar parameter '%s' in the collection"), *M.Name, *Pair.Key);
      Instance->SetScalarParameterValue(Name, FCString::Atof(*Pair.Value));
    }
  }
}
