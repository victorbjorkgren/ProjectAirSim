// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#pragma once

#include <memory>

namespace microsoft::projectairsim {
class Simulator;
}

namespace PlayServices {

// Registers GetPlayRequest, GetManualInput and SetHudStatus on the simulator
// when the play game mode is running; does nothing otherwise. Called on every
// scene load because a scene reload drops every service.
void RegisterAll(std::shared_ptr<microsoft::projectairsim::Simulator> Simulator);

}  // namespace PlayServices
