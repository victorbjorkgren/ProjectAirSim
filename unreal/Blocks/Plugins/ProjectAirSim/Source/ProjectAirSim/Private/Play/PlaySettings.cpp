// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#include "Play/PlaySettings.h"

#include "Play/PlayLog.h"

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
