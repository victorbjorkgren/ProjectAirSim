// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#include "Play/PlayTestScript.h"

#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Play/PlayLog.h"

TUniquePtr<FPlayTestScript> FPlayTestScript::FromEnvironment() {
  FString Path = FPlatformMisc::GetEnvironmentVariable(TEXT("PLAY_TEST_SCRIPT"));
  if (Path.IsEmpty()) FParse::Value(FCommandLine::Get(), TEXT("playtest="), Path);
  if (Path.IsEmpty()) return nullptr;
  return TUniquePtr<FPlayTestScript>(new FPlayTestScript(Path));
}

FPlayTestScript::FPlayTestScript(const FString& Path) {
  TArray<FString> Lines;
  if (!FFileHelper::LoadFileToStringArray(Lines, *Path)) {
    UE_LOG(LogPlay, Fatal, TEXT("Play test script '%s' could not be read"), *Path);
  }
  for (const FString& Raw : Lines) {
    const FString Line = Raw.TrimStartAndEnd();
    if (Line.IsEmpty() || Line.StartsWith(TEXT("#"))) continue;
    FStep Step;
    TArray<FString> Parts;
    Line.ParseIntoArrayWS(Parts);
    Step.Verb = Parts[0];
    Parts.RemoveAt(0);
    Step.Args = Parts;
    Steps.Add(Step);
  }
  UE_LOG(LogPlay, Log, TEXT("Play test script '%s': %d steps"), *Path, Steps.Num());
}

FKey FPlayTestScript::ParseKey(const FString& Name) const {
  const FKey Key(*Name);
  if (!Key.IsValid()) UE_LOG(LogPlay, Fatal, TEXT("Play test script: unknown key '%s'"), *Name);
  return Key;
}

void FPlayTestScript::Tick(const FPlayTestHooks& H) {
  const double Now = FPlatformTime::Seconds();
  for (int32 I = 0; I < KeyQueue.Num();) {
    if (Now >= KeyQueue[I].At) {
      H.Key(KeyQueue[I].Key, KeyQueue[I].bDown);
      KeyQueue.RemoveAt(I);
    } else {
      ++I;
    }
  }
  if (bStickHeld && Now >= StickUntil) {
    H.ClearStick();
    bStickHeld = false;
  }
  if (!WaitScreen.IsEmpty()) {
    if (H.ScreenName() == WaitScreen) {
      WaitScreen.Reset();
    } else if (Now > ScreenDeadline) {
      UE_LOG(LogPlay, Fatal, TEXT("Play test: timed out waiting for screen %s (on %s)"), *WaitScreen, *H.ScreenName());
    } else {
      return;
    }
  }
  while (Next < Steps.Num() && Now >= ResumeAt && WaitScreen.IsEmpty()) {
    const FStep& S = Steps[Next++];
    UE_LOG(LogPlay, Log, TEXT("Play test step %d: %s %s"), Next, *S.Verb, *FString::Join(S.Args, TEXT(" ")));
    if (S.Verb == TEXT("wait")) {
      ResumeAt = Now + FCString::Atod(*S.Args[0]);
    } else if (S.Verb == TEXT("key")) {
      const int32 Count = S.Args.Num() > 1 ? FCString::Atoi(*S.Args[1]) : 1;
      const FKey Key = ParseKey(S.Args[0]);
      for (int32 I = 0; I < Count; ++I) {
        KeyQueue.Add({Now + I * 0.3, Key, true});
        KeyQueue.Add({Now + I * 0.3 + 0.12, Key, false});
      }
      ResumeAt = Now + Count * 0.3 + 0.2;
    } else if (S.Verb == TEXT("keydown") || S.Verb == TEXT("keyup")) {
      H.Key(ParseKey(S.Args[0]), S.Verb == TEXT("keydown"));
    } else if (S.Verb == TEXT("expect")) {
      if (H.ScreenName() != S.Args[0]) {
        UE_LOG(LogPlay, Fatal, TEXT("Play test: expected screen %s, on %s"), *S.Args[0], *H.ScreenName());
      }
    } else if (S.Verb == TEXT("waitscreen")) {
      WaitScreen = S.Args[0];
      ScreenDeadline = Now + FCString::Atod(*S.Args[1]);
    } else if (S.Verb == TEXT("shot")) {
      FString Dir = FPlatformMisc::GetEnvironmentVariable(TEXT("PLAY_TEST_SHOT_DIR"));
      if (Dir.IsEmpty()) Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("PlayShots"));
      H.Screenshot(FPaths::Combine(Dir, FString::Printf(TEXT("%02d_%s.png"), ++ShotCounter, *S.Args[0])));
      ResumeAt = Now + 0.8;
    } else if (S.Verb == TEXT("stick")) {
      FPlayStick Stick;
      Stick.Vx = FCString::Atod(*S.Args[0]);
      Stick.Vy = FCString::Atod(*S.Args[1]);
      Stick.Vz = FCString::Atod(*S.Args[2]);
      Stick.YawRate = FCString::Atod(*S.Args[3]);
      H.Stick(Stick);
      bStickHeld = true;
      StickUntil = Now + FCString::Atod(*S.Args[4]);
      ResumeAt = StickUntil;
    } else if (S.Verb == TEXT("takeoff")) {
      H.TakeoffLand();
      ResumeAt = Now + 0.2;
    } else if (S.Verb == TEXT("log")) {
      UE_LOG(LogPlay, Log, TEXT("Play test: %s | screen=%s hud=[%s]"), *FString::Join(S.Args, TEXT(" ")),
             *H.ScreenName(), *H.HudText());
    } else if (S.Verb == TEXT("quit")) {
      H.Quit();
    } else {
      UE_LOG(LogPlay, Fatal, TEXT("Play test script: unknown command '%s'"), *S.Verb);
    }
  }
}
