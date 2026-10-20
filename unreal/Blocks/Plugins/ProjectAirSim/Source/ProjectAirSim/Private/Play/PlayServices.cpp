// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#include "Play/PlayServices.h"

#include "Play/PlayLog.h"
#include "Play/PlayShared.h"
#include "core_sim/clock.hpp"
#include "core_sim/service_method.hpp"
#include "core_sim/simulator.hpp"

namespace projectairsim = microsoft::projectairsim;

void PlayServices::RegisterAll(std::shared_ptr<projectairsim::Simulator> Simulator) {
  TSharedPtr<FPlayShared, ESPMode::ThreadSafe> Shared = FPlayShared::Get();
  if (!Shared.IsValid()) return;
  check(Simulator);

  auto get_request = projectairsim::ServiceMethod("GetPlayRequest", {});
  Simulator->RegisterServiceMethod(
      get_request, get_request.CreateMethodHandler(std::function<nlohmann::json()>(
                       [Shared]() { return Shared->RequestJson(); })));

  auto get_input = projectairsim::ServiceMethod("GetManualInput", {});
  Simulator->RegisterServiceMethod(
      get_input, get_input.CreateMethodHandler(std::function<nlohmann::json()>([Shared]() {
        const double SimSeconds = projectairsim::SimClock::Get()->NowSimNanos() / 1.0e9;
        return Shared->ManualInputJson(SimSeconds);
      })));

  auto set_hud = projectairsim::ServiceMethod("SetHudStatus", {"text", "level"});
  Simulator->RegisterServiceMethod(
      set_hud, set_hud.CreateMethodHandler(std::function<bool(std::string, std::string)>(
                   [Shared](std::string Text, std::string Level) {
                     Shared->SetHud(UTF8_TO_TCHAR(Text.c_str()), UTF8_TO_TCHAR(Level.c_str()));
                     return true;
                   })));

  UE_LOG(LogPlay, Log, TEXT("Registered play services GetPlayRequest, GetManualInput, SetHudStatus"));
}
