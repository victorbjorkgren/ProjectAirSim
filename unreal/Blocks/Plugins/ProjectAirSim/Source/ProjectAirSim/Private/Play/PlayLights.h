// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Play/PlaySettings.h"

class AActor;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UProceduralMeshComponent;
class USceneComponent;
class USpotLightComponent;
class UWorld;

enum class EPlayLightMode : uint8 { Steady, Strobe, Pulse };

// What the player chose for the drone's lights. The game mode, the HUD and the LIGHTS panel share one instance; the
// panel edits the colours in place and then asks the game mode to apply and save them.
struct FPlayLightsState {
  bool bFlashlight = false;
  bool bNav = false;
  EPlayLightMode Mode = EPlayLightMode::Steady;
  // Index into the settings' presets, or INDEX_NONE for colours edited by hand.
  int32 Preset = 0;
  // sRGB. The tail light exists only while bTail is set.
  FColor Left = FColor::White;
  FColor Right = FColor::White;
  FColor Tail = FColor::White;
  bool bTail = true;
  // Written by FPlayLights: the project's own spot light on the robot makes the flashlight yield.
  bool bFlashlightYielded = false;

  FString PresetName(const UPlaySettings& Settings) const;
};

// The drone's lights: a flashlight (spot light) and left / right / tail position lights (a point light and an
// emissive glow each), attached to whichever robot the play mode is flying. The lights belong to the robot actor, so a
// scene reload takes them away and the next Tick builds them again. Nothing here names a project's actors: where the
// lights sit comes from the robot's own bounds or from socket names in the settings.
class FPlayLights {
 public:
  // StorePath: the ini file the chosen colours and pattern persist in.
  FPlayLights(const UPlaySettings& InSettings, const FString& StorePath);

  TSharedRef<FPlayLightsState> State() const { return StateRef; }

  // Call every frame with the robot being flown (null when there is none) and wall-clock seconds; the lights follow
  // wall time because the scene pauses the world between simulation steps.
  void Tick(UWorld& World, AActor* Robot, double WallSeconds);

  // The state changed (a toggle, a preset, an edited colour): lights are re-coloured next Tick and the choice is saved.
  void Changed();
  void ToggleFlashlight();
  void ToggleNav();
  void CycleMode();
  void CyclePreset();
  void ToggleTail();
  void SetPreset(int32 Index);
  // A mood's starting lights (see FPlayMoodEntry::Flashlight / NavLights).
  void ApplyMood(const FPlayMoodEntry& Mood);

  // One-line summary for the HUD.
  FString HudText() const;
  // Brightness factor (0..1.x) of a light at a time, for the lights and the panel's preview. Role: 0 left, 1 right, 2 tail.
  static float Pattern(EPlayLightMode Mode, int32 Role, bool bHasTail, double Seconds);
  static const TCHAR* ModeName(EPlayLightMode Mode);

 private:
  struct FNavLight {
    UPointLightComponent* Light = nullptr;
    UProceduralMeshComponent* Glow = nullptr;
    UMaterialInstanceDynamic* Material = nullptr;
  };

  void Load();
  void Save() const;
  void Attach(UWorld& World, AActor& Robot);
  void Detach();
  FNavLight MakeNav(AActor& Robot, const TCHAR* Name, const FVector& Bounds, const FString& Socket, const FBox& Box);
  USceneComponent* SocketParent(AActor& Robot, const FString& Socket) const;
  FBox LocalBounds(AActor& Robot) const;
  FVector BoundsPoint(const FBox& Box, const FVector& Fraction) const;
  bool HasForeignSpot(AActor& Robot) const;
  void UpdateColours(double Seconds);

  const UPlaySettings& Settings;
  FString StorePath;
  TSharedRef<FPlayLightsState> StateRef;
  TWeakObjectPtr<AActor> Attached;
  USpotLightComponent* Spot = nullptr;
  FNavLight Nav[3];
  UMaterialInterface* GlowBase = nullptr;
  double NextForeignCheck = 0.0;
  bool bForeignSpot = false;
};
