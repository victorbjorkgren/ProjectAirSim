// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Play/Audio/PlayAudio.h"
#include "Play/PlayInputMap.h"
#include "Play/PlayMood.h"
#include "Play/PlayShared.h"
#include "Play/PlayTestScript.h"
#include "Play/UI/PlayUi.h"
#include "ProjectAirSimGameMode.h"

#include "PlayGameMode.generated.h"

class APlayCameraActor;
class AUnrealRobot;
class UPlaySettings;

// The manual play mode: the stock ProjectAirSim game mode plus a title screen,
// menus, a HUD, a flight camera and the services a Python attendant uses to
// run scenarios for a human pilot. Selected with ?game=AirSimPlay.
UCLASS()
class AProjectAirSimPlayGameMode : public AProjectAirSimGameMode {
  GENERATED_BODY()

 public:
  explicit AProjectAirSimPlayGameMode(const FObjectInitializer& ObjectInitializer);

  void StartPlay() override;
  void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
  void Tick(float DeltaSeconds) override;

 private:
  void BuildModel();
  void BuildActions();
  void LoadBackdrop(const struct FPlayMapEntry& Map);
  void GatherMarkers();

  void GoTo(EPlayScreen Screen);
  void Launch(const FPlayRunRequest& Request);
  void StopRun();
  void SelectMood(int32 Index);
  void ContinueFromMode();
  void ApplyInputMode(bool bUi);

  AUnrealRobot* FindRobot();
  void RemoveStockTabBinding(APlayerController& Controller);
  void UpdateDriftCenter();
  void TickFlight(APlayerController& Controller, float WallDelta);
  void UpdateHud(float WallDelta);
  void UpdateFootnotes();
  void UpdateLaunchSequence();
  void MuteProjectSounds();
  void LogFrameRate();
  void BuildTestHooks();
  static const TCHAR* ScreenName(EPlayScreen Screen);

  // The screen the player sees, and the choices made on the way to a launch.
  EPlayScreen Screen = EPlayScreen::Intro;
  int32 SelectedMode = 0;
  bool bMoodFromPause = false;
  bool bHasSpawn = false;
  FVector3d SpawnNed = FVector3d::ZeroVector;
  FPlayRunRequest LastRequest;
  double LaunchWallSeconds = 0.0;
  // Launch sequence: when it began, and when the curtain began to lift.
  double IntroStartSeconds = 0.0;
  double CurtainLiftSeconds = -1.0;

  const UPlaySettings* Settings = nullptr;
  int32 CurrentMapIndex = INDEX_NONE;
  FString CurrentLevelName;
  TSharedPtr<FPlayUiModel> Model;
  TSharedPtr<SPlayUi> Ui;
  TSharedPtr<FPlayShared, ESPMode::ThreadSafe> Shared;
  UPROPERTY() TObjectPtr<UPlayAudio> Audio;
  TUniquePtr<FPlayMoodDirector> MoodDirector;
  FPlayInputMap InputMap;
  FPlayUiActions Actions;
  TUniquePtr<FPlayTestScript> TestScript;
  FPlayTestHooks TestHooks;
  bool bStickOverride = false;
  FPlayStick OverrideStick;
  FString LoadingBase;

  UPROPERTY() TObjectPtr<APlayCameraActor> Camera;
  TWeakObjectPtr<AUnrealRobot> Robot;
  TWeakObjectPtr<AUnrealRobot> RobotBeforeLaunch;

  // Flight state
  static constexpr int32 kPresetCount = 4;
  int32 PresetIndex = 1;
  int32 CameraIndex = 0;
  float TiltDegrees = 0.f;
  FVector LastRobotLocation = FVector::ZeroVector;
  bool bHaveLastRobotLocation = false;
  float SpeedMs = 0.f;
  float AltitudeM = 0.f;

  // Bookkeeping
  double LastWallSeconds = 0.0;
  double NextRobotSearch = 0.0;
  double NextTabCheck = 0.0;
  double NextDriftRefresh = 0.0;
  double NextMuteSweep = 0.0;
  double FpsWindowStart = 0.0;
  int32 FpsWindowFrames = 0;
  int32 DriftRefreshes = 0;
  int32 AppliedUiMode = -1;
  bool bMarkersReady = false;
};
