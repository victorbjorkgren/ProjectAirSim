// Copyright (C) Microsoft Corporation.  
// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#include "SpawnPoint.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"

ASpawnPoint::ASpawnPoint() {
  PrimaryActorTick.bCanEverTick = false;

  // Create a root scene component (invisible, just for positioning)
  RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
  RootComponent = RootSceneComponent;

  // Set default display name to actor name
  DisplayName = TEXT("SpawnPoint");
  Description = TEXT("");
}

void ASpawnPoint::BeginPlay() {
  Super::BeginPlay();

  // Ensure the spawn point has a tag so it can be found easily
  if (!ActorHasTag(FName("SpawnPoint"))) {
    Tags.Add(FName("SpawnPoint"));
  }
}

#if WITH_EDITOR
void ASpawnPoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {
  Super::PostEditChangeProperty(PropertyChangedEvent);

  // Ensure the spawn point always has the SpawnPoint tag
  if (!ActorHasTag(FName("SpawnPoint"))) {
    Tags.Add(FName("SpawnPoint"));
  }
}
#endif

