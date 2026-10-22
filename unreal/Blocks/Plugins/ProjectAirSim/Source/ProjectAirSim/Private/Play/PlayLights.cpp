// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#include "Play/PlayLights.h"

#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/ConfigCacheIni.h"
#include "Play/PlayLog.h"
#include "ProceduralMeshComponent.h"

namespace {
const TCHAR* kSection = TEXT("PlayLights");
const FName kOurTag(TEXT("PlayLights"));

// Anti-collision strobe: two short flashes, then a pause, repeating about every 1.2 s.
constexpr double kStrobePeriod = 1.2;
constexpr double kStrobeFlash = 0.07;
constexpr double kStrobeGap = 0.10;
constexpr double kPulsePeriod = 2.4;
// The strobe is brighter than a steady light.
constexpr float kStrobeBoost = 3.f;

FLinearColor ToLinear(const FColor& C) { return FLinearColor::FromSRGBColor(C); }

// A unit UV sphere as a procedural mesh section.
void BuildSphere(UProceduralMeshComponent& Mesh) {
  constexpr int32 Rings = 8;
  constexpr int32 Segments = 14;
  TArray<FVector> Vertices, Normals;
  TArray<FVector2D> Uvs;
  TArray<int32> Triangles;
  for (int32 R = 0; R <= Rings; ++R) {
    const float Phi = PI * R / Rings;
    for (int32 S = 0; S <= Segments; ++S) {
      const float Theta = 2.f * PI * S / Segments;
      const FVector N(FMath::Sin(Phi) * FMath::Cos(Theta), FMath::Sin(Phi) * FMath::Sin(Theta), FMath::Cos(Phi));
      Vertices.Add(N);
      Normals.Add(N);
      Uvs.Add(FVector2D(static_cast<float>(S) / Segments, static_cast<float>(R) / Rings));
    }
  }
  for (int32 R = 0; R < Rings; ++R) {
    for (int32 S = 0; S < Segments; ++S) {
      const int32 A = R * (Segments + 1) + S;
      const int32 B = A + Segments + 1;
      Triangles.Append({A, B, A + 1, A + 1, B, B + 1});
    }
  }
  Mesh.CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, Uvs, TArray<FLinearColor>(),
                                     TArray<FProcMeshTangent>(), false);
}
}  // namespace

FString FPlayLightsState::PresetName(const UPlaySettings& Settings) const {
  return Settings.LightPresets.IsValidIndex(Preset) ? Settings.LightPresets[Preset].Name.ToUpper() : FString(TEXT("CUSTOM"));
}

const TCHAR* FPlayLights::ModeName(EPlayLightMode Mode) {
  switch (Mode) {
    case EPlayLightMode::Steady: return TEXT("STEADY");
    case EPlayLightMode::Strobe: return TEXT("STROBE");
    case EPlayLightMode::Pulse: return TEXT("PULSE");
  }
  UE_LOG(LogPlay, Fatal, TEXT("Unhandled light mode"));
  return TEXT("");
}

float FPlayLights::Pattern(EPlayLightMode Mode, int32 Role, bool bHasTail, double Seconds) {
  switch (Mode) {
    case EPlayLightMode::Steady: return 1.f;
    case EPlayLightMode::Strobe: {
      // The tail light is the strobe; a preset without one strobes its left and right lights instead.
      if (bHasTail && Role != 2) return 1.f;
      if (!bHasTail && Role == 2) return 0.f;
      const double T = FMath::Fmod(Seconds, kStrobePeriod);
      const bool bOn = T < kStrobeFlash || (T >= kStrobeFlash + kStrobeGap && T < 2.0 * kStrobeFlash + kStrobeGap);
      return bOn ? kStrobeBoost : 0.f;
    }
    case EPlayLightMode::Pulse: {
      const double Phase = 2.0 * PI * Seconds / kPulsePeriod + Role * 0.6;
      return 0.55f + 0.45f * static_cast<float>(FMath::Sin(Phase));
    }
  }
  UE_LOG(LogPlay, Fatal, TEXT("Unhandled light mode"));
  return 0.f;
}

FPlayLights::FPlayLights(const UPlaySettings& InSettings, const FString& InStorePath)
    : Settings(InSettings), StorePath(InStorePath), StateRef(MakeShared<FPlayLightsState>()) {
  if (Settings.LightPresets.Num() == 0) UE_LOG(LogPlay, Fatal, TEXT("PlaySettings.LightPresets is empty"));
  Load();
}

void FPlayLights::Load() {
  FPlayLightsState& S = *StateRef;
  S.Preset = 0;
  const FPlayLightPreset& First = Settings.LightPresets[0];
  S.Left = First.Left;
  S.Right = First.Right;
  S.Tail = First.Tail;
  S.bTail = First.Tail.A > 0;
  FConfigFile Ini;
  Ini.Read(StorePath);
  if (!Ini.FindSection(kSection)) return;
  auto Str = [&](const TCHAR* Key) -> FString {
    FString Value;
    Ini.GetString(kSection, Key, Value);
    return Value;
  };
  auto ParseColour = [&](const FString& Text, FColor& Out) {
    if (Text.IsEmpty()) return;
    if (!Out.InitFromString(Text)) UE_LOG(LogPlay, Fatal, TEXT("%s: colour '%s' is not (R=,G=,B=,A=)"), *StorePath, *Text);
  };
  const FString PresetName = Str(TEXT("Preset"));
  if (!PresetName.IsEmpty()) {
    S.Preset = PresetName == TEXT("custom") ? INDEX_NONE
                                            : Settings.LightPresets.IndexOfByPredicate([&](const FPlayLightPreset& P) {
                                                return P.Name.Equals(PresetName, ESearchCase::IgnoreCase);
                                              });
  }
  ParseColour(Str(TEXT("Left")), S.Left);
  ParseColour(Str(TEXT("Right")), S.Right);
  ParseColour(Str(TEXT("Tail")), S.Tail);
  if (!Str(TEXT("HasTail")).IsEmpty()) S.bTail = Str(TEXT("HasTail")) == TEXT("1");
  const FString Mode = Str(TEXT("Mode"));
  if (Mode == TEXT("strobe")) S.Mode = EPlayLightMode::Strobe;
  if (Mode == TEXT("pulse")) S.Mode = EPlayLightMode::Pulse;
  UE_LOG(LogPlay, Log, TEXT("Lights: restored preset %s mode %s from %s"), *S.PresetName(Settings), ModeName(S.Mode), *StorePath);
}

void FPlayLights::Save() const {
  const FPlayLightsState& S = *StateRef;
  FConfigFile Ini;
  Ini.Read(StorePath);
  auto Set = [&](const TCHAR* Key, const FString& Value) { Ini.SetString(kSection, Key, *Value); };
  Set(TEXT("Preset"), S.Preset == INDEX_NONE ? FString(TEXT("custom")) : Settings.LightPresets[S.Preset].Name);
  Set(TEXT("Left"), S.Left.ToString());
  Set(TEXT("Right"), S.Right.ToString());
  Set(TEXT("Tail"), S.Tail.ToString());
  Set(TEXT("HasTail"), S.bTail ? TEXT("1") : TEXT("0"));
  Set(TEXT("Mode"), FString(ModeName(S.Mode)).ToLower());
  Ini.Dirty = true;
  if (!Ini.Write(StorePath)) UE_LOG(LogPlay, Error, TEXT("Lights: could not write %s"), *StorePath);
}

void FPlayLights::Changed() { Save(); }

void FPlayLights::ToggleFlashlight() { StateRef->bFlashlight = !StateRef->bFlashlight; }
void FPlayLights::ToggleNav() { StateRef->bNav = !StateRef->bNav; }

void FPlayLights::CycleMode() {
  FPlayLightsState& S = *StateRef;
  S.Mode = static_cast<EPlayLightMode>((static_cast<uint8>(S.Mode) + 1) % 3);
  Changed();
}

void FPlayLights::SetPreset(int32 Index) {
  check(Settings.LightPresets.IsValidIndex(Index));
  FPlayLightsState& S = *StateRef;
  const FPlayLightPreset& P = Settings.LightPresets[Index];
  S.Preset = Index;
  S.Left = P.Left;
  S.Right = P.Right;
  S.Tail = P.Tail;
  S.bTail = P.Tail.A > 0;
  Changed();
}

void FPlayLights::CyclePreset() {
  const int32 Count = Settings.LightPresets.Num();
  SetPreset((StateRef->Preset + 1 + Count) % Count);
}

void FPlayLights::ToggleTail() {
  StateRef->bTail = !StateRef->bTail;
  StateRef->Preset = INDEX_NONE;
  Changed();
}

void FPlayLights::ApplyMood(const FPlayMoodEntry& Mood) {
  if (Mood.Flashlight >= 0) StateRef->bFlashlight = Mood.Flashlight > 0;
  if (Mood.NavLights >= 0) StateRef->bNav = Mood.NavLights > 0;
}

FString FPlayLights::HudText() const {
  const FPlayLightsState& S = *StateRef;
  FString Flash = S.bFlashlight ? (S.bFlashlightYielded ? TEXT("PROJECT LAMP") : TEXT("ON")) : TEXT("OFF");
  FString Navigation = S.bNav ? FString::Printf(TEXT("%s %s"), *S.PresetName(Settings), ModeName(S.Mode)) : FString(TEXT("OFF"));
  return FString::Printf(TEXT("FLASH(F) %s   NAV(N/B) %s"), *Flash, *Navigation);
}

// ---------------------------------------------------------------------------
// Attaching to a robot

FBox FPlayLights::LocalBounds(AActor& Robot) const {
  const FTransform ToLocal = Robot.GetActorTransform().Inverse();
  FBox Box(ForceInit);
  TArray<UMeshComponent*> Meshes;
  Robot.GetComponents(Meshes);
  for (UMeshComponent* Mesh : Meshes) {
    if (!Mesh->IsVisible() || Mesh->bHiddenInGame || Mesh->ComponentHasTag(kOurTag)) continue;
    Box += Mesh->CalcBounds(Mesh->GetComponentTransform() * ToLocal).GetBox();
  }
  if (!Box.IsValid) {
    UE_LOG(LogPlay, Warning, TEXT("Lights: robot %s has no visible mesh to measure; using LightFallbackHalfExtentCm"),
           *Robot.GetName());
    Box = FBox(-Settings.LightFallbackHalfExtentCm, Settings.LightFallbackHalfExtentCm);
  }
  return Box;
}

FVector FPlayLights::BoundsPoint(const FBox& Box, const FVector& F) const {
  const FVector Centre = Box.GetCenter();
  const FVector Half = Box.GetExtent();
  const FVector Inset(Settings.LightInsetCm);
  // A face is pulled in by the inset; a fraction inside the box scales the half extent.
  FVector P = Centre;
  for (int32 Axis = 0; Axis < 3; ++Axis) {
    const double Reach = FMath::Max(Half[Axis] - Inset[Axis], 0.0);
    P[Axis] = Centre[Axis] + F[Axis] * Reach;
  }
  return P;
}

USceneComponent* FPlayLights::SocketParent(AActor& Robot, const FString& Socket) const {
  TArray<USceneComponent*> Components;
  Robot.GetComponents(Components);
  for (USceneComponent* C : Components) {
    if (!C->ComponentHasTag(kOurTag) && C->DoesSocketExist(FName(*Socket))) return C;
  }
  UE_LOG(LogPlay, Fatal, TEXT("Lights: no component of robot %s has the socket '%s' named in PlaySettings"), *Robot.GetName(), *Socket);
  return nullptr;
}

FPlayLights::FNavLight FPlayLights::MakeNav(AActor& Robot, const TCHAR* Name, const FVector& Fraction, const FString& Socket,
                                            const FBox& Box) {
  USceneComponent* Parent = Socket.IsEmpty() ? Robot.GetRootComponent() : SocketParent(Robot, Socket);
  const FName SocketName = Socket.IsEmpty() ? NAME_None : FName(*Socket);
  const FVector Where = Socket.IsEmpty() ? BoundsPoint(Box, Fraction) : FVector::ZeroVector;
  FNavLight Out;

  Out.Light = NewObject<UPointLightComponent>(&Robot, FName(*FString::Printf(TEXT("PlayNavLight_%s"), Name)));
  Out.Light->ComponentTags.Add(kOurTag);
  Out.Light->SetupAttachment(Parent, SocketName);
  Out.Light->SetRelativeLocation(Where);
  Out.Light->SetIntensityUnits(ELightUnits::Lumens);
  Out.Light->SetIntensity(Settings.NavLightLumens);
  Out.Light->SetAttenuationRadius(Settings.NavLightRadiusCm);
  Out.Light->SetSourceRadius(Settings.NavGlowRadiusCm);
  Out.Light->SetCastShadows(false);
  Out.Light->SetVolumetricScatteringIntensity(1.f);
  Out.Light->RegisterComponent();

  Out.Glow = NewObject<UProceduralMeshComponent>(&Robot, FName(*FString::Printf(TEXT("PlayNavGlow_%s"), Name)));
  Out.Glow->ComponentTags.Add(kOurTag);
  Out.Glow->SetupAttachment(Parent, SocketName);
  Out.Glow->SetRelativeLocation(Where);
  Out.Glow->SetRelativeScale3D(FVector(Settings.NavGlowRadiusCm));
  Out.Glow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Out.Glow->SetCastShadow(false);
  Out.Glow->bUseAsyncCooking = false;
  BuildSphere(*Out.Glow);
  Out.Material = UMaterialInstanceDynamic::Create(GlowBase, Out.Glow);
  Out.Glow->SetMaterial(0, Out.Material);
  Out.Glow->RegisterComponent();
  return Out;
}

void FPlayLights::Attach(UWorld&, AActor& Robot) {
  if (!GlowBase) {
    GlowBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial"));
    if (!GlowBase) UE_LOG(LogPlay, Fatal, TEXT("Lights: /Engine/EngineMaterials/EmissiveMeshMaterial could not be loaded"));
    GlowBase->AddToRoot();
  }
  const FBox Box = LocalBounds(Robot);
  UE_LOG(LogPlay, Log, TEXT("Lights: attaching to %s, local bounds min %s max %s"), *Robot.GetName(), *Box.Min.ToString(), *Box.Max.ToString());

  Nav[0] = MakeNav(Robot, TEXT("Left"), Settings.NavLeftBounds, Settings.NavLeftSocket, Box);
  Nav[1] = MakeNav(Robot, TEXT("Right"), Settings.NavRightBounds, Settings.NavRightSocket, Box);
  Nav[2] = MakeNav(Robot, TEXT("Tail"), Settings.NavTailBounds, Settings.NavTailSocket, Box);

  USceneComponent* Parent = Settings.FlashlightSocket.IsEmpty() ? Robot.GetRootComponent() : SocketParent(Robot, Settings.FlashlightSocket);
  const FName SocketName = Settings.FlashlightSocket.IsEmpty() ? NAME_None : FName(*Settings.FlashlightSocket);
  Spot = NewObject<USpotLightComponent>(&Robot, TEXT("PlayFlashlight"));
  Spot->ComponentTags.Add(kOurTag);
  Spot->SetupAttachment(Parent, SocketName);
  // On the nose, a few centimetres ahead of the front face so the airframe does not shadow its own beam.
  FVector Nose = Settings.FlashlightSocket.IsEmpty() ? BoundsPoint(Box, Settings.FlashlightBounds) : FVector::ZeroVector;
  if (Settings.FlashlightSocket.IsEmpty()) Nose.X += Settings.LightInsetCm + 6.0;
  Spot->SetRelativeLocation(Nose);
  Spot->SetRelativeRotation(FRotator(-Settings.FlashlightPitchDeg, 0.f, 0.f));
  Spot->SetIntensityUnits(ELightUnits::Lumens);
  Spot->SetIntensity(Settings.FlashlightLumens);
  Spot->SetOuterConeAngle(Settings.FlashlightOuterConeDeg);
  Spot->SetInnerConeAngle(Settings.FlashlightInnerConeDeg);
  Spot->SetAttenuationRadius(Settings.FlashlightRangeCm);
  Spot->SetSourceRadius(2.f);
  Spot->SetLightColor(ToLinear(Settings.FlashlightColor));
  Spot->SetCastShadows(Settings.bFlashlightShadows);
  Spot->SetVolumetricScatteringIntensity(1.f);
  Spot->RegisterComponent();
  Attached = &Robot;
  NextForeignCheck = 0.0;
}

void FPlayLights::Detach() {
  // A robot that is still alive (the play mode moved on to another) loses its lights; a destroyed one took them along.
  if (Attached.IsValid()) {
    if (Spot) Spot->DestroyComponent();
    for (FNavLight& N : Nav) {
      if (N.Light) N.Light->DestroyComponent();
      if (N.Glow) N.Glow->DestroyComponent();
    }
  }
  Spot = nullptr;
  for (FNavLight& N : Nav) N = FNavLight();
  Attached.Reset();
}

bool FPlayLights::HasForeignSpot(AActor& Robot) const {
  TArray<USpotLightComponent*> Spots;
  Robot.GetComponents(Spots);
  for (USpotLightComponent* S : Spots) {
    if (S->ComponentHasTag(kOurTag)) continue;
    if (S->IsVisible() && S->Intensity > 0.f) return true;
  }
  return false;
}

void FPlayLights::UpdateColours(double Seconds) {
  const FPlayLightsState& S = *StateRef;
  const FColor Colours[3] = {S.Left, S.Right, S.Tail};
  for (int32 Role = 0; Role < 3; ++Role) {
    FNavLight& N = Nav[Role];
    const bool bPresent = S.bNav && (Role != 2 || S.bTail);
    const float Factor = bPresent ? Pattern(S.Mode, Role, S.bTail, Seconds) : 0.f;
    const FLinearColor Colour = ToLinear(Colours[Role]);
    N.Light->SetVisibility(Factor > 0.01f);
    N.Light->SetLightColor(Colour);
    N.Light->SetIntensity(Settings.NavLightLumens * Factor);
    N.Glow->SetVisibility(Factor > 0.01f);
    N.Material->SetVectorParameterValue(TEXT("Color"), Colour * (Settings.NavGlowBrightness * Factor));
  }
}

void FPlayLights::Tick(UWorld& World, AActor* Robot, double WallSeconds) {
  if (Attached.IsValid() && Attached.Get() != Robot) Detach();
  if (!Robot) return;
  if (!Attached.IsValid()) Attach(World, *Robot);

  FPlayLightsState& S = *StateRef;
  if (WallSeconds >= NextForeignCheck) {
    NextForeignCheck = WallSeconds + 0.5;
    bForeignSpot = HasForeignSpot(*Robot);
  }
  S.bFlashlightYielded = bForeignSpot;
  const bool bOn = S.bFlashlight && !bForeignSpot;
  Spot->SetVisibility(bOn);
  UpdateColours(WallSeconds);
}
