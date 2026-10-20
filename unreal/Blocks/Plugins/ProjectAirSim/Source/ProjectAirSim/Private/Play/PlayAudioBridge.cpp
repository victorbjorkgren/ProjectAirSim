// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#include "Play/PlayAudioBridge.h"

#include "Play/PlayLog.h"

namespace {
// Silent stand-in until the procedural audio layer is wired in.
class FSilentPlayAudio : public IPlayAudio {
 public:
  void MenuAmbient(bool) override {}
  void FlightAmbient(bool) override {}
  void UiClick() override {}
  void UiConfirm() override {}
  void RotorLoop(float) override {}
};
}  // namespace

TUniquePtr<IPlayAudio> CreatePlayAudio(UWorld*) {
  UE_LOG(LogPlay, Log, TEXT("Play audio: silent"));
  return MakeUnique<FSilentPlayAudio>();
}
