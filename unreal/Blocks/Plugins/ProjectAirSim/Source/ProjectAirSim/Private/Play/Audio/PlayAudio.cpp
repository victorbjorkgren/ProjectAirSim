// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#include "PlayAudio.h"

#include "HAL/IConsoleManager.h"
#include "Misc/Paths.h"
#include "Play/Audio/PlayAudioDsp.h"

DEFINE_LOG_CATEGORY_STATIC(LogPlayAudio, Log, All);

namespace {

constexpr int32 kDumpSampleRate = 48000;

// play.audio.dump <seconds> <wav path>: renders every sound through PlayAudioDsp::RenderSound, the code
// the game mixer runs, to <path without .wav>_menu.wav, _flight.wav, _click.wav, _confirm.wav,
// _rotor_low.wav and _rotor_high.wav.
void DumpSounds(const TArray<FString>& Args) {
  if (Args.Num() != 2) {
    UE_LOG(LogPlayAudio, Error, TEXT("usage: play.audio.dump <seconds> <wav path>"));
    return;
  }
  const float Seconds = FCString::Atof(*Args[0]);
  if (Seconds <= 0.0f || Seconds > 600.0f) {
    UE_LOG(LogPlayAudio, Error, TEXT("play.audio.dump: seconds must be in (0, 600], got '%s'"), *Args[0]);
    return;
  }
  const FString Prefix = FPaths::ChangeExtension(Args[1], TEXT(""));
  using PlayAudioDsp::Sound;
  for (Sound S : {Sound::Menu, Sound::Flight, Sound::Click, Sound::Confirm, Sound::RotorLow,
                  Sound::RotorHigh}) {
    const FString Path = Prefix + UTF8_TO_TCHAR(PlayAudioDsp::SoundSuffix(S));
    try {
      PlayAudioDsp::WriteWav(TCHAR_TO_UTF8(*Path),
                             PlayAudioDsp::RenderSound(S, Seconds, kDumpSampleRate), kDumpSampleRate);
      UE_LOG(LogPlayAudio, Display, TEXT("play.audio.dump: wrote %s"), *Path);
    } catch (const std::exception& E) {
      UE_LOG(LogPlayAudio, Error, TEXT("play.audio.dump: %s"), UTF8_TO_TCHAR(E.what()));
      return;
    }
  }
}

FAutoConsoleCommand DumpCommand(
    TEXT("play.audio.dump"),
    TEXT("play.audio.dump <seconds> <wav path>: render the play-mode sounds to WAV files."),
    FConsoleCommandWithArgsDelegate::CreateStatic(&DumpSounds));

}  // namespace

UPlayAudio::UPlayAudio(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer), Controls(MakeUnique<PlayAudioDsp::Controls>()) {
  NumChannels = 2;
  bAutoActivate = true;
  bAllowSpatialization = false;
  bIsUISound = true;
}

UPlayAudio::~UPlayAudio() = default;

bool UPlayAudio::Init(int32& SampleRate) {
  Mixer = MakeUnique<PlayAudioDsp::Mixer>(static_cast<float>(SampleRate), *Controls);
  return true;
}

int32 UPlayAudio::OnGenerateAudio(float* OutAudio, int32 NumSamples) {
  check(Mixer);
  Mixer->Render(OutAudio, NumSamples / NumChannels);
  return NumSamples;
}

void UPlayAudio::MenuAmbient(bool bOn) { Controls->menu_on.store(bOn); }

void UPlayAudio::FlightAmbient(bool bOn) { Controls->flight_on.store(bOn); }

void UPlayAudio::UiClick() { Controls->click_count.fetch_add(1); }

void UPlayAudio::UiConfirm() { Controls->confirm_count.fetch_add(1); }

void UPlayAudio::RotorLoop(float Throttle01) { Controls->rotor_throttle.store(Throttle01); }
