// Copyright (C) Microsoft Corporation.  
// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.
// The Unreal sensor base.
// It is currently empty and could potentially be replaced by USceneComponent.

#pragma once

#include "Components/SceneComponent.h"
#include "CoreMinimal.h"
#include "core_sim/clock.hpp"

// comment so that generated.h is always the last include file with clang-format
#include "UnrealSensor.generated.h"

UCLASS()
class UUnrealSensor : public USceneComponent {
  GENERATED_BODY()

 public:
  explicit UUnrealSensor(const FObjectInitializer& ObjectInitializer)
      : USceneComponent(ObjectInitializer) {
    // GetWorld() should only be called on the game thread, so call it in the
    // sensor's constructor to save it
    UnrealWorld = this->GetWorld();
  }

  // Sets the transform relative to the attach parent, with the translation in
  // metres of the parent's frame: the parent's world scale (a link's visual
  // mesh scale) does not scale the sensor's origin or the sensor itself.
  void SetRelativeTransformIgnoringParentScale(const FTransform& Desired) {
    SetRelativeTransform(Desired);
    CancelParentScale();
  }

  // Counters the attach parent's world scale on the current relative
  // transform, which must be unscaled: the translation is divided by the
  // parent scale component-wise and the relative scale becomes its inverse, so
  // the sensor's world scale stays 1. Identity when there is no parent or the
  // parent's scale is 1.
  void CancelParentScale() {
    const USceneComponent* Parent = GetAttachParent();
    if (Parent == nullptr) return;
    FVector ParentScale = Parent->GetComponentScale();
    for (int32 Axis = 0; Axis < 3; ++Axis) {
      if (FMath::IsNearlyZero(ParentScale[Axis])) ParentScale[Axis] = 1.0;
    }
    if (ParentScale.Equals(FVector::OneVector)) return;
    const FVector InverseScale = FVector::OneVector / ParentScale;
    SetRelativeLocation(GetRelativeLocation() * InverseScale);
    SetRelativeScale3D(InverseScale);
  }

  void SetHasNewState(bool bNewState) { bHasNewState = bNewState; }

  void SetSimTimeAtPoseUpdate(TimeNano SimTimeNano) {
    PoseUpdatedTimeStamp = SimTimeNano;
  }

 protected:
  bool bHasNewState = false;
  TimeNano PoseUpdatedTimeStamp = 0;
  UWorld* UnrealWorld = nullptr;

 private:
};
