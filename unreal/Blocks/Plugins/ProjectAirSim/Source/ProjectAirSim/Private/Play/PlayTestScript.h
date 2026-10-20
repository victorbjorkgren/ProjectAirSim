// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Play/PlayShared.h"

// What a test script can do to the running play mode.
struct FPlayTestHooks {
  TFunction<void(const FKey&, bool bDown)> Key;
  TFunction<void(const FString& Path)> Screenshot;
  TFunction<void(const FPlayStick&)> Stick;      // overrides the keyboard sticks
  TFunction<void()> ClearStick;
  TFunction<void()> TakeoffLand;
  TFunction<FString()> ScreenName;
  TFunction<FString()> HudText;
  TFunction<void()> Quit;
};

// A scripted run of the menus and the flight sticks, for verification without
// a person at the keyboard. Enabled by PLAY_TEST_SCRIPT=<file> in the
// environment or -playtest=<file> on the command line; one command per line,
// '#' starts a comment:
//   wait <seconds>
//   key <Name> [count]            a key press and release (Down, Enter, Escape, Tab, W ...)
//   keydown <Name> / keyup <Name>
//   expect <screen>               title|map|mode|spawn|vehicle|pause|loading|flying; a mismatch is fatal
//   waitscreen <screen> <timeout> waits for a screen; a timeout is fatal
//   shot <name>                   PNG with the UI to $PLAY_TEST_SHOT_DIR (default Saved/PlayShots)
//   stick <vx> <vy> <vz> <yaw_rate> <seconds>   holds that stick state, then releases it
//   takeoff                       the take-off/land press
//   log <text...>                 writes the line and the HUD text to the log
//   quit
class FPlayTestScript {
 public:
  // Null when no script is configured.
  static TUniquePtr<FPlayTestScript> FromEnvironment();

  void Tick(const FPlayTestHooks& Hooks);

 private:
  struct FStep {
    FString Verb;
    TArray<FString> Args;
  };
  explicit FPlayTestScript(const FString& Path);
  FKey ParseKey(const FString& Name) const;

  TArray<FStep> Steps;
  int32 Next = 0;
  double ResumeAt = 0.0;
  double ScreenDeadline = 0.0;
  double StickUntil = 0.0;
  bool bStickHeld = false;
  FString WaitScreen;
  int32 ShotCounter = 0;
};
