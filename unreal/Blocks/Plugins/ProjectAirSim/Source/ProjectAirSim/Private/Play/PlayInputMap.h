// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

class APlayerController;

enum class EPlayAxis : uint8 { Forward, Right, Climb, Yaw, CameraTilt };
enum class EPlayButton : uint8 { PresetDown, PresetUp, TakeoffLand, CycleCamera, Pause };

struct FPlayAxisBinding {
  EPlayAxis Axis;
  FKey Positive;
  FKey Negative;
};

struct FPlayButtonBinding {
  EPlayButton Button;
  FKey Key;
};

// What one frame of input says. Axes are -1..1; a button is set on the frame
// its key goes down.
struct FPlayInputSample {
  float Axes[5] = {0, 0, 0, 0, 0};
  bool Buttons[5] = {false, false, false, false, false};

  float Axis(EPlayAxis A) const { return Axes[static_cast<int32>(A)]; }
  bool Pressed(EPlayButton B) const { return Buttons[static_cast<int32>(B)]; }
};

// The single mapping table from keys to flight input. A gamepad adds rows
// here and nowhere else.
class FPlayInputMap {
 public:
  FPlayInputMap();

  // Reads the controller's key state; a button edge needs the previous call's
  // state, so call once per frame.
  FPlayInputSample Poll(const APlayerController& Controller);

  // Forgets held keys, so a key held through a menu is not an edge on return.
  void Reset();

  const TArray<FPlayAxisBinding>& Axes() const { return AxisBindings; }
  const TArray<FPlayButtonBinding>& Buttons() const { return ButtonBindings; }

 private:
  TArray<FPlayAxisBinding> AxisBindings;
  TArray<FPlayButtonBinding> ButtonBindings;
  bool WasDown[5] = {false, false, false, false, false};
};
