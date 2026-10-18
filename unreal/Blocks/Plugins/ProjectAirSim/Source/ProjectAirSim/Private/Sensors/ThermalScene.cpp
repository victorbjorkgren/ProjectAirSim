// Copyright (C) 2026 Nordic Flight Stack
//
// MIT License. All rights reserved.

#include "ThermalScene.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "Components/DirectionalLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Landscape.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "LandscapeComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UnrealLogger.h"
#include "core_sim/json_utils.hpp"

namespace projectairsim = microsoft::projectairsim;
using json = nlohmann::json;

namespace {

constexpr int32 kMaxClasses = 256;

// A class with all its fields, for the built-in table below.
FThermalClass Make(const TCHAR* Name, float Emissivity, float Absorptivity,
                   float Inertia, float Convection, float InternalTempC = 0.0f,
                   float InternalWeight = 0.0f, float Snow = 1.0f) {
  FThermalClass C;
  C.Name = Name;
  C.Emissivity = Emissivity;
  C.SolarAbsorptivity = Absorptivity;
  C.Inertia = Inertia;
  C.Convection = Convection;
  C.InternalTempC = InternalTempC;
  C.InternalWeight = InternalWeight;
  C.SnowSusceptibility = Snow;
  return C;
}

float JsonNumber(const json& Object, const char* Key, float Default) {
  return Object.contains(Key) ? Object.at(Key).get<float>() : Default;
}

}  // namespace

FThermalScene& FThermalScene::Get() {
  static FThermalScene Instance;
  return Instance;
}

FThermalScene::FThermalScene() {
  Classes.SetNum(kMaxClasses);
  Classes[0] = Make(TEXT("generic"), 0.95f, 0.65f, 0.3f, 1.0f);

  // Emissivities: Wolfe & Zissis, Mikhail 2001 and ASTER spectral library band means for 8-14 um;
  // absorptivities are broadband solar; inertia is a qualitative 0..1 from volumetric heat capacity.
  const FThermalClass Builtins[] = {
      Make(TEXT("foliage"), 0.97f, 0.50f, 0.05f, 2.5f, 0, 0, 0.5f),
      Make(TEXT("wood"), 0.94f, 0.65f, 0.40f, 1.0f, 0, 0, 0.6f),
      Make(TEXT("litter"), 0.96f, 0.75f, 0.45f, 0.8f),
      Make(TEXT("soil"), 0.93f, 0.75f, 0.80f, 0.9f),
      Make(TEXT("rock"), 0.91f, 0.70f, 0.95f, 1.0f),
      Make(TEXT("water"), 0.985f, 0.06f, 1.00f, 1.0f, FThermalClass::kWaterTemp, 0.85f, 0.0f),
      Make(TEXT("snow"), 0.985f, 0.15f, 0.25f, 1.0f),
      Make(TEXT("asphalt"), 0.95f, 0.90f, 0.85f, 1.0f),
      Make(TEXT("concrete"), 0.92f, 0.60f, 0.85f, 1.0f),
      Make(TEXT("masonry"), 0.93f, 0.65f, 0.80f, 1.0f, 0, 0, 0.3f),
      Make(TEXT("roof"), 0.90f, 0.80f, 0.45f, 1.2f),
      Make(TEXT("metal_bare"), 0.15f, 0.40f, 0.30f, 1.0f, 0, 0, 0.7f),
      Make(TEXT("vehicle_body"), 0.88f, 0.60f, 0.25f, 1.2f, 0, 0, 0.7f),
      Make(TEXT("glass"), 0.90f, 0.15f, 0.20f, 1.0f, 0, 0, 0.1f),
      Make(TEXT("tyre"), 0.95f, 0.90f, 0.50f, 1.0f, 30.0f, 0.3f, 0.2f),
      Make(TEXT("engine"), 0.90f, 0.80f, 0.60f, 1.0f, 75.0f, 0.75f, 0.0f),
      Make(TEXT("motor"), 0.90f, 0.70f, 0.20f, 1.5f, 55.0f, 0.85f, 0.0f),
      Make(TEXT("battery"), 0.92f, 0.70f, 0.40f, 1.0f, 38.0f, 0.7f, 0.0f),
      Make(TEXT("drone_body"), 0.92f, 0.70f, 0.20f, 1.5f, 30.0f, 0.35f, 0.3f),
      Make(TEXT("human_skin"), 0.98f, 0.60f, 0.10f, 1.0f, 34.0f, 0.95f, 0.0f),
      Make(TEXT("human"), 0.96f, 0.60f, 0.10f, 1.0f, 28.0f, 0.80f, 0.2f),
      Make(TEXT("animal"), 0.97f, 0.60f, 0.10f, 1.0f, 31.0f, 0.85f, 0.2f),
      Make(TEXT("hot_object"), 0.95f, 0.80f, 0.30f, 1.0f, 150.0f, 1.0f, 0.0f),
  };
  for (const FThermalClass& C : Builtins) RegisterClass(C);

  // Default name patterns. A project's mapping file is checked after tags and
  // before these, so it can override any of them.
  struct { const TCHAR* Class; const TCHAR* Material; const TCHAR* Mesh; } Defaults[] = {
      {TEXT("asphalt"), TEXT("*asphalt*"), TEXT("")},
      {TEXT("asphalt"), TEXT(""), TEXT("*road*")},
      {TEXT("concrete"), TEXT("*concrete*"), TEXT("")},
      {TEXT("concrete"), TEXT("*pavement*"), TEXT("")},
      {TEXT("concrete"), TEXT("*sidewalk*"), TEXT("")},
      {TEXT("glass"), TEXT("*glass*"), TEXT("")},
      {TEXT("glass"), TEXT("*window*"), TEXT("")},
      {TEXT("metal_bare"), TEXT("*chrome*"), TEXT("")},
      {TEXT("metal_bare"), TEXT("*steel*"), TEXT("")},
      {TEXT("metal_bare"), TEXT("*alumin*"), TEXT("")},
      {TEXT("tyre"), TEXT("*tire*"), TEXT("")},
      {TEXT("tyre"), TEXT("*tyre*"), TEXT("")},
      {TEXT("masonry"), TEXT("*brick*"), TEXT("")},
      {TEXT("roof"), TEXT("*roof*"), TEXT("")},
      {TEXT("water"), TEXT("*water*"), TEXT("")},
      {TEXT("snow"), TEXT("*snow*"), TEXT("")},
      {TEXT("rock"), TEXT("*rock*"), TEXT("")},
      {TEXT("rock"), TEXT("*stone*"), TEXT("")},
      {TEXT("soil"), TEXT("*dirt*"), TEXT("")},
      {TEXT("soil"), TEXT("*soil*"), TEXT("")},
      {TEXT("soil"), TEXT("*mud*"), TEXT("")},
      {TEXT("soil"), TEXT("*sand*"), TEXT("")},
      {TEXT("foliage"), TEXT("*leaf*"), TEXT("")},
      {TEXT("foliage"), TEXT("*leaves*"), TEXT("")},
      {TEXT("foliage"), TEXT("*foliage*"), TEXT("")},
      {TEXT("foliage"), TEXT("*grass*"), TEXT("")},
      {TEXT("wood"), TEXT("*bark*"), TEXT("")},
      {TEXT("wood"), TEXT("*wood*"), TEXT("")},
      {TEXT("human_skin"), TEXT("*skin*"), TEXT("")},
      {TEXT("vehicle_body"), TEXT("*carpaint*"), TEXT("")},
      {TEXT("vehicle_body"), TEXT(""), TEXT("*vehicle*")},
  };
  for (auto& D : Defaults) {
    FRule Rule;
    Rule.ClassId = FindClass(D.Class);
    Rule.Material = D.Material;
    Rule.Mesh = D.Mesh;
    Rules.Add(Rule);
  }
}

int32 FThermalScene::RegisterClass(const FThermalClass& Class) {
  int32 Id = FindClass(Class.Name);
  if (Id == 0) {
    for (int32 I = 1; I < kMaxClasses; ++I) {
      if (Classes[I].Name.IsNone()) { Id = I; break; }
    }
    if (Id == 0) throw std::runtime_error("FThermalScene: more than 255 thermal classes");
  }
  Classes[Id] = Class;
  bTableDirty = true;
  return Id;
}

int32 FThermalScene::FindClass(FName Name) const {
  for (int32 I = 1; I < kMaxClasses; ++I) {
    if (Classes[I].Name == Name) return I;
  }
  return 0;
}

void FThermalScene::SetEnvironment(const FThermalEnvironment& NewEnvironment) {
  if (FMemory::Memcmp(&Environment, &NewEnvironment, sizeof(FThermalEnvironment)) == 0) return;
  Environment = NewEnvironment;
  bTableDirty = true;
}

void FThermalScene::LoadMapping(const FString& Path) {
  // A packaged Mac app is sandboxed: a relative path is read from the project's Content
  // directory, where a file staged as non-UFS lives (DirectoriesToAlwaysStageAsNonUFS).
  FString Resolved = Path;
  if (FPaths::IsRelative(Resolved)) Resolved = FPaths::Combine(FPaths::ProjectContentDir(), Resolved);
  FString Text;
  if (!FFileHelper::LoadFileToString(Text, *Resolved)) {
    throw std::runtime_error(std::string("Thermal mapping file not readable: ") + TCHAR_TO_UTF8(*Resolved));
  }
  json Doc = json::parse(std::string(TCHAR_TO_UTF8(*Text)));

  if (Doc.contains("environment")) {
    const json& E = Doc.at("environment");
    FThermalEnvironment Env = Environment;
    Env.AirTempC = JsonNumber(E, "air-temp-c", Env.AirTempC);
    Env.SkyDepressionK = JsonNumber(E, "sky-depression-k", Env.SkyDepressionK);
    Env.CloudCover = JsonNumber(E, "cloud-cover", Env.CloudCover);
    Env.SolarIrradiance = JsonNumber(E, "solar-irradiance", Env.SolarIrradiance);
    Env.SolarSlowIrradiance = JsonNumber(E, "solar-slow-irradiance", Env.SolarSlowIrradiance);
    Env.WindSpeed = JsonNumber(E, "wind-speed", Env.WindSpeed);
    Env.ExtinctionPerM = JsonNumber(E, "extinction-per-m", Env.ExtinctionPerM);
    Env.SnowCover = JsonNumber(E, "snow-cover", Env.SnowCover);
    Env.WaterTempC = JsonNumber(E, "water-temp-c", Env.WaterTempC);
    SetEnvironment(Env);
  }

  if (Doc.contains("classes")) {
    for (auto& [Name, C] : Doc.at("classes").items()) {
      const FName ClassName(UTF8_TO_TCHAR(Name.c_str()));
      FThermalClass Base = Classes[FindClass(ClassName)];  // unknown name: generic's values
      Base.Name = ClassName;
      Base.Emissivity = JsonNumber(C, "emissivity", Base.Emissivity);
      Base.SolarAbsorptivity = JsonNumber(C, "solar-absorptivity", Base.SolarAbsorptivity);
      Base.Inertia = JsonNumber(C, "inertia", Base.Inertia);
      Base.Convection = JsonNumber(C, "convection", Base.Convection);
      Base.InternalTempC = JsonNumber(C, "internal-temp-c", Base.InternalTempC);
      Base.InternalWeight = JsonNumber(C, "internal-weight", Base.InternalWeight);
      Base.SnowSusceptibility = JsonNumber(C, "snow-susceptibility", Base.SnowSusceptibility);
      RegisterClass(Base);
    }
  }

  if (Doc.contains("rules")) {
    TArray<FRule> Added;
    for (const json& R : Doc.at("rules")) {
      FRule Rule;
      const std::string ClassName = R.at("class").get<std::string>();
      Rule.ClassId = FindClass(FName(UTF8_TO_TCHAR(ClassName.c_str())));
      if (Rule.ClassId == 0) throw std::runtime_error("Thermal mapping rule names unknown class: " + ClassName);
      auto Field = [&R](const char* Key) {
        return R.contains(Key) ? FString(UTF8_TO_TCHAR(R.at(Key).get<std::string>().c_str())) : FString();
      };
      Rule.Mesh = Field("mesh");
      Rule.Material = Field("material");
      Rule.PhysicalMaterial = Field("physical-material");
      Rule.Actor = Field("actor");
      if (Rule.Mesh.IsEmpty() && Rule.Material.IsEmpty() && Rule.PhysicalMaterial.IsEmpty() && Rule.Actor.IsEmpty())
        throw std::runtime_error("Thermal mapping rule for " + ClassName + " has no match field");
      Added.Add(Rule);
    }
    // The file's rules come before the built-in patterns.
    Rules.Insert(Added, 0);
  }
  // Rescan: classes may now be assigned differently.
  ScannedWorld.Reset();
}

void FThermalScene::AssignClass(UPrimitiveComponent* Component, int32 Id) {
  if (!Component || Id <= 0 || Id >= kMaxClasses) return;
  Component->SetCustomDepthStencilValue(Id);
  Component->SetCustomDepthStencilWriteMask(ERendererStencilMask::ERSM_Default);
  Component->SetRenderCustomDepth(true);
}

int32 FThermalScene::ClassifyComponent(UPrimitiveComponent* Component) const {
  // 1. An explicit tag on the component, then on its actor: "thermal:<class>".
  auto FromTags = [this](const TArray<FName>& Tags) -> int32 {
    for (const FName& Tag : Tags) {
      FString S = Tag.ToString();
      if (S.RemoveFromStart(TEXT("thermal:"), ESearchCase::IgnoreCase)) {
        const int32 Id = FindClass(FName(*S));
        if (Id != 0) return Id;
        UnrealLogger::Log(projectairsim::LogLevel::kWarning,
                          TEXT("[Thermal] Tag '%s' names an unknown thermal class."), *Tag.ToString());
      }
    }
    return 0;
  };
  if (int32 Id = FromTags(Component->ComponentTags)) return Id;
  AActor* Owner = Component->GetOwner();
  if (Owner) {
    if (int32 Id = FromTags(Owner->Tags)) return Id;
  }

  // 2. Rules by name.
  FString MeshName, ActorName;
  if (Owner) ActorName = Owner->GetActorNameOrLabel();
  if (const UObject* Asset = Component->AdditionalStatObject()) MeshName = Asset->GetName();

  TArray<FString> MaterialNames, PhysNames;
  const int32 NumMaterials = Component->GetNumMaterials();
  for (int32 I = 0; I < NumMaterials; ++I) {
    if (UMaterialInterface* M = Component->GetMaterial(I)) {
      MaterialNames.Add(M->GetName());
      if (UMaterial* Base = M->GetMaterial()) MaterialNames.Add(Base->GetName());
      if (UPhysicalMaterial* P = M->GetPhysicalMaterial()) PhysNames.Add(P->GetName());
    }
  }
  auto Matches = [](const FString& Pattern, const TArray<FString>& Names) {
    for (const FString& N : Names) {
      if (N.MatchesWildcard(Pattern, ESearchCase::IgnoreCase)) return true;
    }
    return false;
  };
  for (const FRule& Rule : Rules) {
    if (!Rule.Mesh.IsEmpty() && !MeshName.MatchesWildcard(Rule.Mesh, ESearchCase::IgnoreCase)) continue;
    if (!Rule.Actor.IsEmpty() && !ActorName.MatchesWildcard(Rule.Actor, ESearchCase::IgnoreCase)) continue;
    if (!Rule.Material.IsEmpty() && !Matches(Rule.Material, MaterialNames)) continue;
    if (!Rule.PhysicalMaterial.IsEmpty() && !Matches(Rule.PhysicalMaterial, PhysNames)) continue;
    return Rule.ClassId;
  }
  // 3. Landscape is ground.
  if (Cast<ULandscapeComponent>(Component)) return FindClass(TEXT("soil"));
  return 0;
}

void FThermalScene::ScanSome(UWorld* World) {
  if (ScannedWorld.Get() != World) {
    ScannedWorld = World;
    ScanQueue.Reset();
    for (TActorIterator<AActor> It(World); It; ++It) ScanQueue.Add(*It);
    ScanCursor = 0;
    Classified = 0;
    if (SpawnHandle.IsValid()) SpawnHandle.Reset();
    SpawnHandle = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda(
        [this](AActor* Actor) { ScanQueue.Add(Actor); }));
  }
  // Spread the scan over frames: a city has hundreds of thousands of components.
  const double Deadline = FPlatformTime::Seconds() + 0.002;
  while (ScanCursor < ScanQueue.Num()) {
    if (AActor* Actor = ScanQueue[ScanCursor].Get()) {
      TInlineComponentArray<UPrimitiveComponent*> Components(Actor);
      for (UPrimitiveComponent* C : Components) {
        const int32 Id = ClassifyComponent(C);
        if (Id != 0) { AssignClass(C, Id); ++Classified; }
      }
    }
    ++ScanCursor;
    if ((ScanCursor & 63) == 0 && FPlatformTime::Seconds() > Deadline) break;
  }
  if (ScanCursor == ScanQueue.Num() && ScanCursor > 0) {
    ScanQueue.Reset();
    ScanCursor = 0;
  }
}

void FThermalScene::UpdateAutoEnvironment(UWorld* World, double SimTimeSec) {
  // The sun is the directional light that is the atmosphere's sun (the moon is not); a scene
  // without one uses its strongest directional light.
  if (!Sun.IsValid() || Sun->GetWorld() != World || (FPlatformTime::Seconds() - SunSearchTime) > 5.0) {
    SunSearchTime = FPlatformTime::Seconds();
    UDirectionalLightComponent* Best = nullptr;
    for (TObjectIterator<UDirectionalLightComponent> It; It; ++It) {
      if (It->GetWorld() != World) continue;
      const bool bAtmosphere = It->IsUsedAsAtmosphereSunLight() && It->GetAtmosphereSunLightIndex() == 0;
      if (!Best || (bAtmosphere && !Best->IsUsedAsAtmosphereSunLight()) ||
          (bAtmosphere == Best->IsUsedAsAtmosphereSunLight() && It->Intensity > Best->Intensity)) {
        Best = *It;
      }
    }
    Sun = Best;
  }
  float SunElevation = -90.0f;
  SunLux = 1.0f;
  if (Sun.IsValid()) {
    SunLux = FMath::Max(Sun->Intensity, 1e-3f);
    SunElevation = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(-Sun->GetForwardVector().Z, -1.0f, 1.0f)));
  }
  const float SinEl = FMath::Sin(FMath::DegreesToRadians(SunElevation));
  const float Cloud = FMath::Clamp(Environment.CloudCover, 0.0f, 1.0f);

  if (Environment.SolarIrradiance >= 0.0f) {
    SolarNow = Environment.SolarIrradiance;
  } else if (SinEl <= 0.0f) {
    SolarNow = 0.0f;
  } else {
    // Direct-normal irradiance through a clear atmosphere (about 835 W/m2 overhead), dimmed by cloud.
    SolarNow = 1000.0f * FMath::Exp(-0.18f / FMath::Max(SinEl, 0.03f)) * (1.0f - 0.75f * Cloud * Cloud * Cloud);
  }

  if (Environment.SolarSlowIrradiance >= 0.0f) {
    SolarSlow = Environment.SolarSlowIrradiance;
  } else if (!bSlowInitialised) {
    SolarSlow = 0.8f * SolarNow;
    bSlowInitialised = true;
  } else if (SimTimeSec > LastSimTimeSec) {
    constexpr double kTau = 7200.0;  // the ground and rocks follow the day's average
    SolarSlow += static_cast<float>((SolarNow - SolarSlow) * (1.0 - FMath::Exp(-(SimTimeSec - LastSimTimeSec) / kTau)));
  }
  LastSimTimeSec = SimTimeSec;
}

void FThermalScene::RebuildTable() {
  if (!Table.IsValid()) {
    UTexture2D* T = UTexture2D::CreateTransient(kMaxClasses, 2, PF_A32B32G32R32F);
    T->Filter = TF_Nearest;
    T->SRGB = false;
    T->NeverStream = true;
    T->AddressX = T->AddressY = TA_Clamp;
    Table.Reset(T);
  }
  FTexture2DMipMap& Mip = Table->GetPlatformData()->Mips[0];
  float* Data = static_cast<float*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
  for (int32 I = 0; I < kMaxClasses; ++I) {
    const FThermalClass& C = Classes[I];
    float TempK = 0.0f;
    if (C.InternalWeight > 0.0f) {
      TempK = (C.InternalTempC == FThermalClass::kWaterTemp ? Environment.WaterTempC : C.InternalTempC) + 273.15f;
    }
    float* Row0 = Data + I * 4;
    float* Row1 = Data + (kMaxClasses + I) * 4;
    Row0[0] = C.Emissivity; Row0[1] = C.SolarAbsorptivity; Row0[2] = C.Inertia; Row0[3] = C.Convection;
    Row1[0] = TempK; Row1[1] = C.InternalWeight; Row1[2] = C.SnowSusceptibility; Row1[3] = 0.0f;
  }
  Mip.BulkData.Unlock();
  Table->UpdateResource();
  bTableDirty = false;
}

void FThermalScene::PrepareCapture(UWorld* World, double SimTimeSec, UMaterialInstanceDynamic* Material) {
  ScanSome(World);
  UpdateAutoEnvironment(World, SimTimeSec);
  if (bTableDirty || !Table.IsValid()) RebuildTable();

  const float Cloud = FMath::Clamp(Environment.CloudCover, 0.0f, 1.0f);
  Material->SetScalarParameterValue(TEXT("AirK"), Environment.AirTempC + 273.15f);
  Material->SetScalarParameterValue(TEXT("SkyDepressionK"), Environment.SkyDepressionK * (1.0f - 0.85f * Cloud));
  Material->SetScalarParameterValue(TEXT("SolarNow"), SolarNow);
  Material->SetScalarParameterValue(TEXT("SolarSlow"), SolarSlow);
  Material->SetScalarParameterValue(TEXT("SunLux"), SunLux);
  Material->SetScalarParameterValue(TEXT("WindMS"), Environment.WindSpeed);
  Material->SetScalarParameterValue(TEXT("ExtinctionPerM"), Environment.ExtinctionPerM);
  Material->SetScalarParameterValue(TEXT("SnowCover"), Environment.SnowCover);
  Material->SetTextureParameterValue(TEXT("ClassTable"), Table.Get());
}
