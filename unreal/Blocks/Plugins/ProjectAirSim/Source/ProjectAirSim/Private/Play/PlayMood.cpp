// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#include "Play/PlayMood.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/Scene.h"
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
  return (bHadParams && !ParamInstance.IsValid()) || (bHadSun && !Sun.IsValid()) || (bHadSky && !Sky.IsValid()) ||
         (bHadFog && !Fog.IsValid());
}

void FPlayMoodDirector::WarnMissing() {
  if (bWarned) return;
  bWarned = true;
  if (!bHadSun) UE_LOG(LogPlay, Warning, TEXT("Mood: the level has no DirectionalLight actor; sun fields are ignored"));
  if (!bHadSky) UE_LOG(LogPlay, Warning, TEXT("Mood: the level has no SkyLight actor; sky fields are ignored"));
  if (!bHadFog) UE_LOG(LogPlay, Warning, TEXT("Mood: the level has no ExponentialHeightFog actor; fog fields are ignored"));
}

bool FPlayMoodDirector::FindActors(UWorld& World) {
  bool bChanged = false;
  bHadSun = bHadSun && Sun.IsValid();
  bHadSky = bHadSky && Sky.IsValid();
  bHadFog = bHadFog && Fog.IsValid();
  bHadParams = bHadParams && ParamInstance.IsValid();
  if (!Sun.IsValid()) {
    if (ADirectionalLight* Found = PickSun(World)) {
      Sun = Found;
      bHadSun = true;
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
      bHadSky = true;
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
      bHadFog = true;
      UExponentialHeightFogComponent* C = It->GetComponent();
      FogBase = {C->FogDensity, C->FogHeightFalloff, C->FogInscatteringLuminance, C->StartDistance};
      UE_LOG(LogPlay, Log, TEXT("Mood: height fog %s base density %.4f falloff %.3f max opacity %.2f start %.0f z %.0f volumetric %d"),
             *It->GetName(), FogBase.Density, FogBase.HeightFalloff, C->FogMaxOpacity, C->StartDistance,
             It->GetActorLocation().Z, C->bEnableVolumetricFog);
      bChanged = true;
      break;
    }
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
      bHadParams = true;
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
  WarnMissing();
  CurrentIndex = Index;
  const FPlayMoodEntry& Mood = Settings.Moods[Index];
  ApplyToActors(World, Mood);
  ApplyGrade(World, Mood);
  ApplyParams(World, Mood);
  ApplyHidden(World, Mood);
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
    C->SetStartDistance(IsSet(M.FogStartDistanceM) ? M.FogStartDistanceM * 100.f : FogBase.StartDistance);
  }
}

void FPlayMoodDirector::ApplyGrade(UWorld&, const FPlayMoodEntry& M) {
  FPostProcessSettings S;
  if (IsSet(M.ExposureBiasEv)) { S.bOverride_AutoExposureBias = true; S.AutoExposureBias = M.ExposureBiasEv; }
  if (IsSet(M.ExposureMinEv)) { S.bOverride_AutoExposureMinBrightness = true; S.AutoExposureMinBrightness = M.ExposureMinEv; }
  if (IsSet(M.ExposureMaxEv)) { S.bOverride_AutoExposureMaxBrightness = true; S.AutoExposureMaxBrightness = M.ExposureMaxEv; }
  if (IsSet(M.WhiteTempK)) {
    S.bOverride_TemperatureType = true;
    S.TemperatureType = ETemperatureMethod::TEMP_WhiteBalance;
    S.bOverride_WhiteTemp = true;
    S.WhiteTemp = M.WhiteTempK;
  }
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
  GradeSink(S);
}

void FPlayMoodDirector::ApplyHidden(UWorld& World, const FPlayMoodEntry& M) {
  for (const TPair<TWeakObjectPtr<UPrimitiveComponent>, bool>& Entry : HiddenByMood) {
    if (UPrimitiveComponent* C = Entry.Key.Get()) C->SetHiddenInGame(Entry.Value);
  }
  HiddenByMood.Reset();
  if (M.HideMeshMaterial.IsEmpty()) return;
  for (TObjectIterator<UStaticMeshComponent> It; It; ++It) {
    UStaticMeshComponent* C = *It;
    if (C->GetWorld() != &World) continue;
    bool bMatch = false;
    for (int32 I = 0; I < C->GetNumMaterials() && !bMatch; ++I) {
      const UMaterialInterface* Material = C->GetMaterial(I);
      bMatch = Material && Material->GetMaterial()->GetName().Contains(M.HideMeshMaterial);
    }
    if (!bMatch) continue;
    HiddenByMood.Emplace(C, static_cast<bool>(C->bHiddenInGame));
    C->SetHiddenInGame(true);
    UE_LOG(LogPlay, Log, TEXT("Mood '%s': hid %s at %s"), *M.Name, *C->GetPathName(), *C->GetComponentLocation().ToString());
  }
  if (HiddenByMood.Num() == 0) UE_LOG(LogPlay, Warning, TEXT("Mood '%s': no mesh uses a material named '%s'"), *M.Name, *M.HideMeshMaterial);
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
