// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#include "Play/PlaySettings.h"

#include "Play/PlayLog.h"

UPlaySettings::UPlaySettings() {
  auto Add = [this](const TCHAR* Name, FColor Left, FColor Right, FColor Tail) {
    FPlayLightPreset& P = LightPresets.AddDefaulted_GetRef();
    P.Name = Name;
    P.Left = Left;
    P.Right = Right;
    P.Tail = Tail;
  };
  const FColor White(255, 255, 255, 255);
  const FColor NoTail(0, 0, 0, 0);
  Add(TEXT("Aviation"), FColor(255, 16, 16), FColor(16, 255, 48), White);
  Add(TEXT("Yellow and blue"), FColor(255, 196, 0), FColor(32, 96, 255), White);
  Add(TEXT("Blue and pink"), FColor(40, 120, 255), FColor(255, 56, 190), White);
  Add(TEXT("Cyan and magenta"), FColor(0, 235, 255), FColor(255, 0, 190), FColor(255, 255, 255));
  Add(TEXT("Orange and teal"), FColor(255, 112, 0), FColor(0, 210, 170), FColor(255, 224, 190));
  Add(TEXT("Acid and violet"), FColor(150, 255, 0), FColor(150, 64, 255), NoTail);
  Add(TEXT("White and red"), FColor(255, 255, 255), FColor(255, 24, 24), FColor(255, 24, 24));
}

TArray<TPair<FString, FString>> ParsePlayOpts(const FString& Opts) {
  TArray<TPair<FString, FString>> Pairs;
  TArray<FString> Items;
  Opts.ParseIntoArray(Items, TEXT(","), true);
  for (const FString& Item : Items) {
    FString Key, Value;
    if (!Item.Split(TEXT("="), &Key, &Value)) {
      UE_LOG(LogPlay, Fatal, TEXT("PlaySettings opts entry '%s' is not key=value"), *Item);
    }
    Pairs.Emplace(Key.TrimStartAndEnd(), Value.TrimStartAndEnd());
  }
  return Pairs;
}
