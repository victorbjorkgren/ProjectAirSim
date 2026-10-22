// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "PlaySettings.generated.h"

// One map the build ships. Level is the map's package name without the path
// (Small_City_LVL); the play menu enables the entry whose Level is loaded.
USTRUCT()
struct FPlayMapEntry {
  GENERATED_BODY()

  UPROPERTY() FString Name;
  // Informational for the menu; the attendant's environment owns the scene
  // config it loads.
  UPROPERTY() FString SceneConfig;
  UPROPERTY() FString Level;
  // Look options for the launcher (for example -npgmood=GoldenHour).
  UPROPERTY() FString Args;
  // Optional spawn-picker backdrop: a PNG path (relative to the project
  // Content directory) and the NED metres it covers as "minX,minY,maxX,maxY"
  // (x north, y east); the image's top edge is maxX.
  UPROPERTY() FString Image;
  UPROPERTY() FString ImageBoundsNed;
};

USTRUCT()
struct FPlayVehicleEntry {
  GENERATED_BODY()

  UPROPERTY() FString Name;
  // The robot config name the attendant spawns.
  UPROPERTY() FString RobotConfig;
};

USTRUCT()
struct FPlayModeEntry {
  GENERATED_BODY()

  UPROPERTY() FString Name;
  UPROPERTY() FString Scenario;
  // "key=value,key=value", forwarded as the request's opts. A value that
  // parses as JSON (1500, true) is sent typed, anything else as a string.
  UPROPERTY() FString Opts;
};

USTRUCT()
struct FPlaySpawnEntry {
  GENERATED_BODY()

  UPROPERTY() FString Map;
  UPROPERTY() FString Name;
  // NED metres.
  UPROPERTY() float X = 0.f;
  UPROPERTY() float Y = 0.f;
  UPROPERTY() float Z = 0.f;
};

// One mood: the look of the level (sun, sky, fog, grade). A field left at its
// "unset" value keeps what the level shipped with, so a mood lists only what
// it changes. The play mode finds the level's DirectionalLight, SkyLight and
// ExponentialHeightFog actors by class (a level whose sun and sky are
// components of a blueprint has none: those fields are ignored with a warning,
// and Commands can drive the project's own director) and grades the play
// camera; no project actor is named here.
USTRUCT()
struct FPlayMoodEntry {
  GENERATED_BODY()

  static constexpr float kUnset = -1000.f;

  UPROPERTY() FString Name;
  // Sun: elevation above the horizon and Unreal yaw (degrees), light units of
  // the level (lux for a photometric level), colour temperature in kelvin and
  // a tint multiplied onto it, disc size in degrees (large = soft shadows).
  UPROPERTY() float SunElevationDeg = kUnset;
  UPROPERTY() float SunYawDeg = kUnset;
  UPROPERTY() float SunIntensity = kUnset;
  UPROPERTY() float SunTemperatureK = kUnset;
  UPROPERTY() FLinearColor SunTint = FLinearColor(0.f, 0.f, 0.f, -1.f);
  UPROPERTY() float SunSourceAngleDeg = kUnset;
  // Sky light: all a mirror surface reflects in a level without reflection
  // captures, and what lights every shadow.
  UPROPERTY() float SkyIntensity = kUnset;
  UPROPERTY() FLinearColor SkyTint = FLinearColor(0.f, 0.f, 0.f, -1.f);
  UPROPERTY() float FogDensity = kUnset;
  UPROPERTY() float FogHeightFalloff = kUnset;
  // Metres from the camera before the fog begins (the level's own start may be hundreds of metres).
  UPROPERTY() float FogStartDistanceM = kUnset;
  UPROPERTY() FLinearColor FogColor = FLinearColor(0.f, 0.f, 0.f, -1.f);
  // Grade (post process of the play camera, so the drone cameras are not
  // graded): exposure bias in EV, uniform saturation and contrast, shadow and
  // highlight gain tints. No grain, vignette or bloom.
  UPROPERTY() float ExposureBiasEv = kUnset;
  UPROPERTY() float ExposureMinEv = kUnset;
  UPROPERTY() float ExposureMaxEv = kUnset;
  UPROPERTY() float Saturation = kUnset;
  UPROPERTY() float Contrast = kUnset;
  // White balance in kelvin; the camera assumes this illuminant, so a higher value warms the picture (6500 is neutral).
  UPROPERTY() float WhiteTempK = kUnset;
  UPROPERTY() FLinearColor ShadowTint = FLinearColor(0.f, 0.f, 0.f, -1.f);
  UPROPERTY() FLinearColor HighlightTint = FLinearColor(0.f, 0.f, 0.f, -1.f);
  // Scalars and colours of the settings' MoodParamCollection, "Name=1.5;Tint=(R=1,G=0.5,B=0.2,A=1)".
  UPROPERTY() FString Params;
  // Console commands run after the look is applied, separated by ';' (for
  // example a project's own weather director: "npg.Mood GoldenHour").
  UPROPERTY() FString Commands;
};

// Play-mode content for one project, read from its DefaultGame.ini section
// [/Script/ProjectAirSim.PlaySettings].
UCLASS(config = Game, defaultconfig)
class UPlaySettings : public UObject {
  GENERATED_BODY()

 public:
  UPROPERTY(Config) FString Title = TEXT("NORDIC FLIGHT");
  // The scenario environment the attendant runs ("citysample", "npg").
  UPROPERTY(Config) FString Env;
  UPROPERTY(Config) TArray<FPlayMapEntry> Maps;
  UPROPERTY(Config) TArray<FPlayVehicleEntry> Vehicles;
  UPROPERTY(Config) TArray<FPlayModeEntry> Modes;
  UPROPERTY(Config) TArray<FPlaySpawnEntry> Spawns;
  // Title-screen drift camera: orbit radius and height above the ground
  // under the orbit centre, metres.
  UPROPERTY(Config) float DriftRadiusM = 70.f;
  UPROPERTY(Config) float DriftHeightM = 45.f;
  // Sounds the project plays on its own (game music) are stopped while the
  // play mode runs when their asset name contains one of these strings.
  UPROPERTY(Config) TArray<FString> MutedSounds;
  // Moods the menus offer; with none listed the mood step and the pause
  // menu entry do not appear. DefaultMood names the one applied at start
  // (-playmood=NAME on the command line wins; "none" or an empty DefaultMood keeps the level's own look).
  UPROPERTY(Config) TArray<FPlayMoodEntry> Moods;
  UPROPERTY(Config) FString DefaultMood;
  // A MaterialParameterCollection asset path (/Game/Lighting/Material/MPC_sky)
  // that moods may write through their Params.
  UPROPERTY(Config) FString MoodParamCollection;
};

// Parses "k=v,k=v" into pairs; an entry without '=' is an error.
TArray<TPair<FString, FString>> ParsePlayOpts(const FString& Opts);
