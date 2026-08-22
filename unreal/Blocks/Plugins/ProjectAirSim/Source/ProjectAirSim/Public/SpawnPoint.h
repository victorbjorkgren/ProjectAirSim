// Copyright (C) Microsoft Corporation.  
// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpawnPoint.generated.h"

/**
 * SpawnPoint is an invisible actor that can be placed in the Unreal editor
 * to mark spawn locations for drones. It can be queried by name to get
 * spawn positions without hardcoding coordinates.
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTAIRSIM_API ASpawnPoint : public AActor {
  GENERATED_BODY()

 public:
  ASpawnPoint();

  // Optional display name for the spawn point (shown in editor)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Point")
  FString DisplayName;

  // Optional description/notes for this spawn point
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Point", meta = (MultiLine = true))
  FString Description;

 protected:
  virtual void BeginPlay() override;

#if WITH_EDITOR
  virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

 private:
  // Root component (invisible scene component)
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
  class USceneComponent* RootSceneComponent;
};

