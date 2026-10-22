// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Play/PlaySettings.h"

class ADirectionalLight;
struct FPostProcessSettings;
class ASkyLight;
class AExponentialHeightFog;
class UMaterialParameterCollection;
class UMaterialParameterCollectionInstance;
class UWorld;

// Applies a mood to the running level. The sun, sky light and height fog are
// found by class and restored to the level's own values before each mood, so
// switching never accumulates; the grade goes to the sink. Tick() re-applies the current mood when a scene
// load has replaced the actors it wrote to.
class FPlayMoodDirector {
 public:
  // GradeSink receives the mood's post process settings (the play camera's view).
  FPlayMoodDirector(const UPlaySettings& InSettings, TFunction<void(const FPostProcessSettings&)> InGradeSink)
      : Settings(InSettings), GradeSink(MoveTemp(InGradeSink)) {}

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
    FLinearColor DiscScale;
  };
  struct FSkyBase {
    float Intensity;
    FLinearColor Color;
  };
  struct FFogBase {
    float Density;
    float HeightFalloff;
    FLinearColor Inscattering;
    float StartDistance;
  };

  bool FindActors(UWorld& World);
  bool ActorsStale() const;
  void WarnMissing();
  void ApplyToActors(UWorld& World, const FPlayMoodEntry& Mood);
  void ApplyGrade(UWorld& World, const FPlayMoodEntry& Mood);
  void ApplyParams(UWorld& World, const FPlayMoodEntry& Mood);

  const UPlaySettings& Settings;
  TFunction<void(const FPostProcessSettings&)> GradeSink;
  int32 CurrentIndex = INDEX_NONE;
  // What was found at the last search: a missing class (NPG has no light actors) is not stale, only a lost one is.
  bool bHadSun = false, bHadSky = false, bHadFog = false, bHadParams = false;
  bool bWarned = false;
  double NextCheck = 0.0;

  TWeakObjectPtr<ADirectionalLight> Sun;
  TWeakObjectPtr<ASkyLight> Sky;
  TWeakObjectPtr<AExponentialHeightFog> Fog;
  TWeakObjectPtr<UMaterialParameterCollectionInstance> ParamInstance;
  FSunBase SunBase;
  FSkyBase SkyBase;
  FFogBase FogBase;
  TMap<FName, float> ScalarBase;
  TMap<FName, FLinearColor> VectorBase;
};
