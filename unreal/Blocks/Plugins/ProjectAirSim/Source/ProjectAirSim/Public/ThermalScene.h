// Copyright (C) 2026 Nordic Flight Stack
//
// MIT License. All rights reserved.
// Thermal (LWIR) scene model shared by every thermal camera: the object classes
// (emissivity, solar absorptivity, thermal inertia, internal temperature) and the
// environment (air temperature, sky, sun, wind, snow).

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/StrongObjectPtr.h"

class UWorld;
class AActor;
class UPrimitiveComponent;
class UTexture2D;
class UMaterialInstanceDynamic;
class UDirectionalLightComponent;

/** What a surface is made of, as far as long-wave infrared is concerned. */
struct PROJECTAIRSIM_API FThermalClass {
  FName Name;
  /** Emissivity at normal incidence; 1 - Emissivity is what the surface reflects of its surroundings. */
  float Emissivity = 0.95f;
  /** Fraction of the sunlight reaching the surface that heats it (near-infrared included). */
  float SolarAbsorptivity = 0.65f;
  /** 0 follows the instantaneous sun like foliage, 1 follows the day's average like rock and asphalt. */
  float Inertia = 0.3f;
  /** Scales convective coupling to the air: leaves are 2.5, still ground 0.8. */
  float Convection = 1.0f;
  /** A warm body: surface temperature in Celsius blended in by InternalWeight. kWaterTemp uses the environment's water temperature. */
  float InternalTempC = 0.0f;
  float InternalWeight = 0.0f;
  /** How much of the environment's snow cover settles on the class (0 vertical walls and engines, 1 open ground). */
  float SnowSusceptibility = 1.0f;

  static constexpr float kWaterTemp = -999.0f;
};

/** The weather and sky the thermal cameras see. Fields left at their "auto" value are derived from the scene's directional light and clock. */
struct PROJECTAIRSIM_API FThermalEnvironment {
  float AirTempC = 12.0f;
  /** How much colder than the air the clear zenith looks, in K: ~25 clear and dry, ~3 under thick cloud. */
  float SkyDepressionK = 25.0f;
  /** Cloud cover 0..1: dims direct sun and the sky depression when they are auto. */
  float CloudCover = 0.0f;
  /** Solar irradiance on a fully sunlit surface facing the sun in W/m2; < 0 derives it from the sun's elevation and CloudCover. */
  float SolarIrradiance = -1.0f;
  /** Day-averaged irradiance that high-inertia surfaces follow; < 0 tracks SolarIrradiance with a 2 h time constant. */
  float SolarSlowIrradiance = -1.0f;
  float WindSpeed = 2.0f;
  /** Atmospheric extinction in 1/m (about 0.0003 in clear air, 0.01-0.05 in fog, rain, snowfall). */
  float ExtinctionPerM = 0.0004f;
  /** Snow lying on up-facing surfaces, 0..1. */
  float SnowCover = 0.0f;
  float WaterTempC = 8.0f;
};

class PROJECTAIRSIM_API FThermalScene {
 public:
  static FThermalScene& Get();

  /** Adds or replaces a class by name; returns its id (the primitive's custom stencil value, 1..255). */
  int32 RegisterClass(const FThermalClass& Class);
  /** Id of a class, 0 (generic) when the name is unknown. */
  int32 FindClass(FName Name) const;
  const FThermalClass& GetClass(int32 Id) const { return Classes[Id]; }

  void SetEnvironment(const FThermalEnvironment& NewEnvironment);
  const FThermalEnvironment& GetEnvironment() const { return Environment; }

  /**
   * Adds the classes, rules and environment of a JSON file: {"environment": {...}, "classes": {name: {...}},
   * "rules": [{"class": name, "mesh"|"material"|"physical-material"|"actor"|"label": wildcard}]}.
   * Throws std::runtime_error on a malformed file.
   */
  void LoadMapping(const FString& Path);

  /** Assigns a class to a component now (stencil + custom depth). */
  void AssignClass(UPrimitiveComponent* Component, int32 Id);

  /** Called by every thermal camera before it captures; does the incremental scan of the world, auto environment and material parameters. */
  void PrepareCapture(UWorld* World, double SimTimeSec, UMaterialInstanceDynamic* Material);

  /** Fraction of the world's actors classified (for logs and tests). */
  int32 NumClassified() const { return Classified; }

 private:
  FThermalScene();
  void ScanSome(UWorld* World);
  int32 ClassifyComponent(UPrimitiveComponent* Component) const;
  void UpdateAutoEnvironment(UWorld* World, double SimTimeSec);
  void RebuildTable();

  struct FRule {
    int32 ClassId = 0;
    FString Mesh, Material, PhysicalMaterial, Actor;
  };

  TArray<FThermalClass> Classes;
  TArray<FRule> Rules;
  FThermalEnvironment Environment;

  // Resolved each capture from the sun and clock
  float SolarNow = 0.0f, SolarSlow = 0.0f, SunLux = 1.0f;
  double LastSimTimeSec = -1.0;
  bool bSlowInitialised = false;

  TWeakObjectPtr<UWorld> ScannedWorld;
  TArray<TWeakObjectPtr<AActor>> ScanQueue;
  int32 ScanCursor = 0;
  FDelegateHandle SpawnHandle;
  int32 Classified = 0;

  TWeakObjectPtr<UDirectionalLightComponent> Sun;
  double SunSearchTime = 0.0;

  TStrongObjectPtr<UTexture2D> Table;
  bool bTableDirty = true;
};
