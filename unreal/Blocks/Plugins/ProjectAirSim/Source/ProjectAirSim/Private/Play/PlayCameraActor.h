// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#pragma once

#include "Camera/CameraComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "PlayCameraActor.generated.h"

enum class EPlayCameraMode : uint8 { Drift, Chase, Fpv };

// The play mode's view target: a slow orbit over the map behind the menus, and
// the chase and first-person cameras while flying. It follows the robot actor
// without being attached to it, so a scene reload leaves it alone.
UCLASS()
class APlayCameraActor : public AActor {
  GENERATED_BODY()

 public:
  APlayCameraActor();

  void SetDrift(const FVector& CenterCm, float RadiusCm, float HeightCm);
  void SetTarget(AActor* Target);
  void SetMode(EPlayCameraMode NewMode);
  EPlayCameraMode GetMode() const { return Mode; }
  // Pitch offset on top of the mode's own aim, degrees.
  void SetTilt(float TiltDegrees) { Tilt = TiltDegrees; }
  // The mood's grade, applied to this view only (the drone cameras keep their own exposure).
  void SetGrade(const FPostProcessSettings& Settings);

  // Driven by the game mode with unpaused wall-clock time, because the scene
  // pauses the world between sim steps.
  void Advance(float DeltaSeconds);

 private:
  void AdvanceDrift(float DeltaSeconds);
  void AdvanceFollow(float DeltaSeconds);

  UPROPERTY() TObjectPtr<UCameraComponent> Camera;
  UPROPERTY() TObjectPtr<AActor> Target;

  EPlayCameraMode Mode = EPlayCameraMode::Drift;
  FVector DriftCenter = FVector::ZeroVector;
  float DriftRadius = 7000.f;
  float DriftHeight = 4500.f;
  float DriftAngle = 0.f;
  float Tilt = 0.f;
  bool bFollowSnap = true;
};
