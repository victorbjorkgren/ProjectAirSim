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
};

// Parses "k=v,k=v" into pairs; an entry without '=' is an error.
TArray<TPair<FString, FString>> ParsePlayOpts(const FString& Opts);
