// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#pragma once

#include <atomic>

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"
#include "json.hpp"

// A run the attendant is asked to start. Spawn is NED metres.
struct FPlayRunRequest {
  FString Scenario;
  FString Env;
  bool bHasSpawn = false;
  FVector3d SpawnNed = FVector3d::ZeroVector;
  FString Vehicle;  // empty = the scenario's default
  TArray<TPair<FString, FString>> Opts;
};

// Stick state already scaled by the speed preset; body frame, NED.
struct FPlayStick {
  double Vx = 0.0;
  double Vy = 0.0;
  double Vz = 0.0;
  double YawRate = 0.0;
  int32 Camera = 0;
};

// State shared between the game thread (menus, input) and the service threads
// that answer GetPlayRequest, GetManualInput and SetHudStatus. One instance
// exists while the play game mode runs; it outlives every scene reload.
class FPlayShared {
 public:
  // The live instance, or null when play mode is off.
  static TSharedPtr<FPlayShared, ESPMode::ThreadSafe> Get();
  static void Set(TSharedPtr<FPlayShared, ESPMode::ThreadSafe> Instance);

  // Requests
  int64 PostRun(const FPlayRunRequest& Request);
  int64 PostStop();
  nlohmann::json RequestJson() const;

  // Manual input
  void SetStick(const FPlayStick& Stick);
  // Latches a takeoff/land press; the next GetManualInput carries it once.
  void PressTakeoffLand();
  nlohmann::json ManualInputJson(double SimTimeSeconds);

  // HUD status pushed by the attendant
  void SetHud(const FString& Text, const FString& Level);
  void GetHud(FString& OutText, FString& OutLevel) const;

  // The attendant polls GetPlayRequest; the menus show whether it is there.
  double SecondsSinceAttendantPoll() const;

 private:
  mutable FCriticalSection Mutex;
  int64 Seq = 0;
  FString Action = TEXT("none");
  FPlayRunRequest Run;
  FPlayStick Stick;
  bool bTakeoffLand = false;
  FString HudText;
  FString HudLevel = TEXT("info");
  mutable std::atomic<double> LastPollSeconds{-1.0};
};
