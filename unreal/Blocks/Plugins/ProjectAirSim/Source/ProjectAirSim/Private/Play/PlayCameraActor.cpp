// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#include "Play/PlayCameraActor.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"

namespace {
constexpr float kDriftSecondsPerOrbit = 150.f;
constexpr float kChaseBackCm = 650.f;
constexpr float kChaseUpCm = 260.f;
constexpr float kChasePitchDeg = -12.f;
constexpr float kFpvForwardCm = 25.f;
constexpr float kFollowStiffness = 6.f;
}  // namespace

APlayCameraActor::APlayCameraActor() {
  PrimaryActorTick.bCanEverTick = false;
  USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
  SetRootComponent(Root);
  Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
  Camera->SetupAttachment(Root);
  Camera->bUsePawnControlRotation = false;
  Camera->SetFieldOfView(90.f);
}

void APlayCameraActor::SetDrift(const FVector& CenterCm, float RadiusCm, float HeightCm) {
  DriftCenter = CenterCm;
  DriftRadius = RadiusCm;
  DriftHeight = HeightCm;
}

void APlayCameraActor::SetTarget(AActor* NewTarget) {
  Target = NewTarget;
  bFollowSnap = true;
}

void APlayCameraActor::SetMode(EPlayCameraMode NewMode) {
  if (NewMode == Mode) return;
  Mode = NewMode;
  bFollowSnap = true;
}

void APlayCameraActor::Advance(float DeltaSeconds) {
  if (Mode == EPlayCameraMode::Drift || !Target) {
    AdvanceDrift(DeltaSeconds);
  } else {
    AdvanceFollow(DeltaSeconds);
  }
}

void APlayCameraActor::AdvanceDrift(float DeltaSeconds) {
  DriftAngle = FMath::Fmod(DriftAngle + 2.f * PI * DeltaSeconds / kDriftSecondsPerOrbit, 2.f * PI);
  const float Bob = 400.f * FMath::Sin(DriftAngle * 3.f);
  const FVector Offset(DriftRadius * FMath::Cos(DriftAngle), DriftRadius * FMath::Sin(DriftAngle),
                       DriftHeight + Bob);
  const FVector Location = DriftCenter + Offset;
  // Aim at a point a little above the orbit centre so the horizon stays in view.
  const FVector Aim = DriftCenter + FVector(0.f, 0.f, DriftHeight * 0.3f);
  SetActorLocationAndRotation(Location, (Aim - Location).Rotation());
  Camera->SetFieldOfView(80.f);
}

void APlayCameraActor::AdvanceFollow(float DeltaSeconds) {
  const FVector TargetLocation = Target->GetActorLocation();
  const FRotator Yaw(0.f, Target->GetActorRotation().Yaw, 0.f);
  const FVector Forward = Yaw.Vector();

  FVector DesiredLocation;
  FRotator DesiredRotation;
  if (Mode == EPlayCameraMode::Chase) {
    DesiredLocation = TargetLocation - Forward * kChaseBackCm + FVector(0.f, 0.f, kChaseUpCm);
    // Pull the camera in front of whatever stands between it and the robot.
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(PlayChase), false, Target);
    if (GetWorld()->LineTraceSingleByChannel(Hit, TargetLocation, DesiredLocation, ECC_Camera, Params)) {
      DesiredLocation = Hit.Location + Hit.Normal * 20.f;
    }
    DesiredRotation = FRotator(kChasePitchDeg + Tilt, Yaw.Yaw, 0.f);
  } else {
    DesiredLocation = TargetLocation + Forward * kFpvForwardCm;
    DesiredRotation = FRotator(Tilt, Yaw.Yaw, 0.f);
  }

  if (bFollowSnap || Mode == EPlayCameraMode::Fpv) {
    SetActorLocationAndRotation(DesiredLocation, DesiredRotation);
    bFollowSnap = false;
  } else {
    const float Alpha = 1.f - FMath::Exp(-kFollowStiffness * DeltaSeconds);
    SetActorLocationAndRotation(
        FMath::Lerp(GetActorLocation(), DesiredLocation, Alpha),
        FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaSeconds, kFollowStiffness));
  }
  Camera->SetFieldOfView(Mode == EPlayCameraMode::Fpv ? 100.f : 85.f);
}

void APlayCameraActor::SetGrade(const FPostProcessSettings& Settings) {
  Camera->PostProcessSettings = Settings;
  Camera->PostProcessBlendWeight = 1.f;
}
