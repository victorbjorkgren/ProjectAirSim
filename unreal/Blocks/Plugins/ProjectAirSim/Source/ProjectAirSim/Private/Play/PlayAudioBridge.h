// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class UWorld;

// What the play mode needs from the audio layer. The game mode owns one
// instance, so API mode (no play game mode) never creates a sound source.
class IPlayAudio {
 public:
  virtual ~IPlayAudio() = default;
  virtual void MenuAmbient(bool bOn) = 0;
  virtual void FlightAmbient(bool bOn) = 0;
  virtual void UiClick() = 0;
  virtual void UiConfirm() = 0;
  virtual void RotorLoop(float Throttle01) = 0;
};

TUniquePtr<IPlayAudio> CreatePlayAudio(UWorld* World);
