// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#include "Play/PlayInputMap.h"

#include "GameFramework/PlayerController.h"

FPlayInputMap::FPlayInputMap() {
  AxisBindings = {
      {EPlayAxis::Forward, EKeys::W, EKeys::S},
      {EPlayAxis::Right, EKeys::D, EKeys::A},
      {EPlayAxis::Climb, EKeys::I, EKeys::K},
      {EPlayAxis::Yaw, EKeys::L, EKeys::J},
      {EPlayAxis::CameraTilt, EKeys::U, EKeys::O},
  };
  ButtonBindings = {
      {EPlayButton::PresetDown, EKeys::Q},
      {EPlayButton::PresetUp, EKeys::E},
      {EPlayButton::TakeoffLand, EKeys::SpaceBar},
      {EPlayButton::CycleCamera, EKeys::Tab},
      {EPlayButton::Pause, EKeys::Escape},
  };
}

FPlayInputSample FPlayInputMap::Poll(const APlayerController& Controller) {
  FPlayInputSample Sample;
  for (const FPlayAxisBinding& B : AxisBindings) {
    float Value = 0.f;
    if (Controller.IsInputKeyDown(B.Positive)) Value += 1.f;
    if (Controller.IsInputKeyDown(B.Negative)) Value -= 1.f;
    Sample.Axes[static_cast<int32>(B.Axis)] = Value;
  }
  for (const FPlayButtonBinding& B : ButtonBindings) {
    const int32 I = static_cast<int32>(B.Button);
    const bool Down = Controller.IsInputKeyDown(B.Key);
    Sample.Buttons[I] = Down && !WasDown[I];
    WasDown[I] = Down;
  }
  return Sample;
}

void FPlayInputMap::Reset() {
  for (bool& Down : WasDown) Down = true;
}
