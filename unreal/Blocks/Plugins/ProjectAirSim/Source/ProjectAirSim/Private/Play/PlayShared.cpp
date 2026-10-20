// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#include "Play/PlayShared.h"

#include "Misc/ScopeLock.h"
#include "Play/PlayLog.h"

namespace {
FCriticalSection GInstanceMutex;
TSharedPtr<FPlayShared, ESPMode::ThreadSafe> GInstance;
}  // namespace

TSharedPtr<FPlayShared, ESPMode::ThreadSafe> FPlayShared::Get() {
  FScopeLock Lock(&GInstanceMutex);
  return GInstance;
}

void FPlayShared::Set(TSharedPtr<FPlayShared, ESPMode::ThreadSafe> Instance) {
  FScopeLock Lock(&GInstanceMutex);
  GInstance = MoveTemp(Instance);
}

int64 FPlayShared::PostRun(const FPlayRunRequest& Request) {
  FScopeLock Lock(&Mutex);
  Run = Request;
  Action = TEXT("run");
  bTakeoffLand = false;
  return ++Seq;
}

int64 FPlayShared::PostStop() {
  FScopeLock Lock(&Mutex);
  Action = TEXT("stop");
  bTakeoffLand = false;
  HudText.Reset();
  return ++Seq;
}

nlohmann::json FPlayShared::RequestJson() const {
  LastPollSeconds = FPlatformTime::Seconds();
  FScopeLock Lock(&Mutex);
  nlohmann::json Out;
  Out["seq"] = Seq;
  Out["action"] = TCHAR_TO_UTF8(*Action);
  Out["scenario"] = TCHAR_TO_UTF8(*(Run.Scenario.IsEmpty() ? FString(TEXT("free")) : Run.Scenario));
  Out["env"] = TCHAR_TO_UTF8(*Run.Env);
  if (Run.bHasSpawn) {
    Out["spawn"] = {Run.SpawnNed.X, Run.SpawnNed.Y, Run.SpawnNed.Z};
  } else {
    Out["spawn"] = nullptr;
  }
  if (Run.Vehicle.IsEmpty()) {
    Out["vehicle"] = nullptr;
  } else {
    Out["vehicle"] = TCHAR_TO_UTF8(*Run.Vehicle);
  }
  nlohmann::json Opts = nlohmann::json::object();
  for (const TPair<FString, FString>& Pair : Run.Opts) {
    // A value that parses as JSON (1500, true, [1,2]) goes out typed; anything else is a string.
    const std::string Text = TCHAR_TO_UTF8(*Pair.Value);
    nlohmann::json Value = nlohmann::json::parse(Text, nullptr, false);
    if (Value.is_discarded()) Value = Text;
    Opts[TCHAR_TO_UTF8(*Pair.Key)] = Value;
  }
  Out["opts"] = Opts;
  return Out;
}

void FPlayShared::SetStick(const FPlayStick& NewStick) {
  FScopeLock Lock(&Mutex);
  Stick = NewStick;
}

void FPlayShared::PressTakeoffLand() {
  FScopeLock Lock(&Mutex);
  bTakeoffLand = true;
}

nlohmann::json FPlayShared::ManualInputJson(double SimTimeSeconds) {
  FScopeLock Lock(&Mutex);
  nlohmann::json Out;
  Out["t"] = SimTimeSeconds;
  Out["vx"] = Stick.Vx;
  Out["vy"] = Stick.Vy;
  Out["vz"] = Stick.Vz;
  Out["yaw_rate"] = Stick.YawRate;
  Out["buttons"] = {{"takeoff_land", bTakeoffLand}, {"camera", Stick.Camera}};
  bTakeoffLand = false;
  return Out;
}

void FPlayShared::SetHud(const FString& Text, const FString& Level) {
  if (Level != TEXT("info") && Level != TEXT("warn") && Level != TEXT("result")) {
    throw std::invalid_argument(std::string("SetHudStatus level must be info, warn or result, got '") +
                                TCHAR_TO_UTF8(*Level) + "'");
  }
  FScopeLock Lock(&Mutex);
  HudText = Text;
  HudLevel = Level;
}

void FPlayShared::GetHud(FString& OutText, FString& OutLevel) const {
  FScopeLock Lock(&Mutex);
  OutText = HudText;
  OutLevel = HudLevel;
}

double FPlayShared::SecondsSinceAttendantPoll() const {
  const double Last = LastPollSeconds;
  return Last < 0.0 ? -1.0 : FPlatformTime::Seconds() - Last;
}
