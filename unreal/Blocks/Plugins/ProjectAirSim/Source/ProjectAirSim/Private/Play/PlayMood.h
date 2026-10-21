// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Play/PlaySettings.h"

class ADirectionalLight;
class APostProcessVolume;
class ASkyLight;
class AExponentialHeightFog;
class UMaterialParameterCollection;
class UMaterialParameterCollectionInstance;
class UWorld;

// Applies a mood to the running level. The sun, sky light and height fog are
// found by class and restored to the level's own values before each mood, so
// switching never accumulates; the grade lives in an unbound post process
// volume this class spawns. Tick() re-applies the current mood when a scene
// load has replaced the actors it wrote to.
class FPlayMoodDirector {
 public:
  explicit FPlayMoodDirector(const UPlaySettings& InSettings) : Settings(InSettings) {}

  // Applies Settings.Moods[Index] to World.
  void Apply(UWorld& World, int32 Index);
  // Cheap; call every frame. Re-applies after a scene load replaced an actor or the parameter collection.
  void Tick(UWorld& World);
  int32 Current() const { return CurrentIndex; }

 private:
  struct FSunBase {
    FRotator Rotation;
    float Intensity;
    FLinearColor Color;
    bool bUseTemperature;
    float Temperature;
    float SourceAngle;
  };
  struct FSkyBase {
    float Intensity;
    FLinearColor Color;
  };
  struct FFogBase {
    float Density;
    float HeightFalloff;
    FLinearColor Inscattering;
  };

  bool FindActors(UWorld& World);
  bool ActorsStale() const;
  void ApplyToActors(UWorld& World, const FPlayMoodEntry& Mood);
  void ApplyGrade(UWorld& World, const FPlayMoodEntry& Mood);
  void ApplyParams(UWorld& World, const FPlayMoodEntry& Mood);

  const UPlaySettings& Settings;
  int32 CurrentIndex = INDEX_NONE;
  double NextCheck = 0.0;

  TWeakObjectPtr<ADirectionalLight> Sun;
  TWeakObjectPtr<ASkyLight> Sky;
  TWeakObjectPtr<AExponentialHeightFog> Fog;
  TWeakObjectPtr<APostProcessVolume> Grade;
  TWeakObjectPtr<UMaterialParameterCollectionInstance> ParamInstance;
  FSunBase SunBase;
  FSkyBase SkyBase;
  FFogBase FogBase;
  TMap<FName, float> ScalarBase;
  TMap<FName, FLinearColor> VectorBase;
};
