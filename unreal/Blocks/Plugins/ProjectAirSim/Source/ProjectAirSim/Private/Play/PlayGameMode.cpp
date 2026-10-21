// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#include "Play/PlayGameMode.h"

#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "HAL/PlatformFileManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Play/PlayCameraActor.h"
#include "Play/PlayLog.h"
#include "Play/PlaySettings.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CoreDelegates.h"
#include "Robot/UnrealRobot.h"
#include "UnrealClient.h"
#include "SpawnPoint.h"
#include "Styling/SlateBrush.h"
#include "Widgets/SWeakWidget.h"

DEFINE_LOG_CATEGORY(LogPlay);

namespace {
// Horizontal speeds in m/s for the Q/E presets; climb is half of it.
constexpr float kPresetSpeeds[4] = {2.f, 5.f, 10.f, 20.f};
constexpr float kYawRate = 1.0f;
constexpr float kTiltRateDegPerSec = 40.f;
constexpr float kTiltMinDeg = -30.f;
constexpr float kTiltMaxDeg = 40.f;
// Launch sequence, seconds: the mark builds up, rests, fades; then the curtain lifts off the title.
constexpr double kIntroBuild = 2.0;
constexpr double kIntroHold = 0.4;
constexpr double kIntroFade = 0.6;
constexpr double kCurtainLift = 0.8;

FString LevelStem(const FString& Level) {
  FString Stem = FPaths::GetBaseFilename(Level);
  return Stem;
}

FVector NedToUnrealCm(const FVector3d& Ned) { return FVector(Ned.X * 100.0, Ned.Y * 100.0, -Ned.Z * 100.0); }
}  // namespace

AProjectAirSimPlayGameMode::AProjectAirSimPlayGameMode(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.bStartWithTickEnabled = true;
  // The scene pauses the world between sim steps; the menus must keep running.
  PrimaryActorTick.bTickEvenWhenPaused = true;
}

void AProjectAirSimPlayGameMode::StartPlay() {
  Settings = GetDefault<UPlaySettings>();
  UE_LOG(LogPlay, Log, TEXT("PlaySettings: env='%s' maps=%d vehicles=%d modes=%d spawns=%d"), *Settings->Env,
         Settings->Maps.Num(), Settings->Vehicles.Num(), Settings->Modes.Num(), Settings->Spawns.Num());
  CurrentLevelName = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);
  for (int32 I = 0; I < Settings->Maps.Num(); ++I) {
    if (LevelStem(Settings->Maps[I].Level) == CurrentLevelName) CurrentMapIndex = I;
  }
  if (Settings->Env.IsEmpty()) {
    UE_LOG(LogPlay, Warning, TEXT("PlaySettings.Env is empty; run requests carry an empty env"));
  }

  Shared = MakeShared<FPlayShared, ESPMode::ThreadSafe>();
  Shared->SetDefaultEnv(Settings->Env);
  FPlayShared::Set(Shared);

  BuildModel();
  BuildActions();
  // The only place a sound source is made: API mode never reaches this.
  Audio = NewObject<UPlayAudio>(this, TEXT("PlayAudio"));
  Audio->RegisterComponentWithWorld(GetWorld());
  Audio->Activate(true);

  FActorSpawnParameters Params;
  Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  Camera = GetWorld()->SpawnActor<APlayCameraActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
  check(Camera);
  Camera->SetDrift(FVector::ZeroVector, Settings->DriftRadiusM * 100.f, Settings->DriftHeightM * 100.f);

  // The simulation and its default scene load here; the services register
  // during that load because the shared state already exists.
  Super::StartPlay();

  if (Settings->Moods.Num() > 0) {
    MoodDirector = MakeUnique<FPlayMoodDirector>(*Settings);
    FString Wanted = Settings->DefaultMood;
    FParse::Value(FCommandLine::Get(), TEXT("-playmood="), Wanted);
    // "none" keeps the level's own look (run.sh uses it for a PLAY_SUN override).
    if (!Wanted.IsEmpty() && !Wanted.Equals(TEXT("none"), ESearchCase::IgnoreCase)) {
      const int32 Index = Settings->Moods.IndexOfByPredicate(
          [&](const FPlayMoodEntry& Mood) { return Mood.Name.Equals(Wanted, ESearchCase::IgnoreCase); });
      if (Index == INDEX_NONE) UE_LOG(LogPlay, Fatal, TEXT("Mood '%s' is not in PlaySettings.Moods"), *Wanted);
      SelectMood(Index);
    }
  } else if (FParse::Param(FCommandLine::Get(), TEXT("playmood"))) {
    UE_LOG(LogPlay, Fatal, TEXT("-playmood was given but PlaySettings lists no moods"));
  }

  check(GEngine && GEngine->GameViewport);
  Ui = SNew(SPlayUi).Model(Model).Actions(FPlayUiActions(Actions));
  GEngine->GameViewport->AddViewportWidgetContent(Ui.ToSharedRef(), 100);
  LastWallSeconds = FPlatformTime::Seconds();
  TestScript = FPlayTestScript::FromEnvironment();
  if (TestScript) BuildTestHooks();
  // A scripted run goes straight to the title.
  if (TestScript) {
    GoTo(EPlayScreen::Title);
  } else {
    IntroStartSeconds = LastWallSeconds;
    Model->CurtainAlpha = 1.f;
    GoTo(EPlayScreen::Intro);
  }
}

void AProjectAirSimPlayGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  if (Ui.IsValid() && GEngine && GEngine->GameViewport) {
    GEngine->GameViewport->RemoveViewportWidgetContent(Ui.ToSharedRef());
  }
  Ui.Reset();
  if (Audio) {
    Audio->Stop();
    Audio->DestroyComponent();
    Audio = nullptr;
  }
  FPlayShared::Set(nullptr);
  Shared.Reset();
  Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// Model and actions

void AProjectAirSimPlayGameMode::BuildModel() {
  Model = MakeShared<FPlayUiModel>();
  Model->Title = Settings->Title;
  if (Settings->Maps.Num() == 0) {
    Model->MapNames.Add(CurrentLevelName.ToUpper());
    Model->MapEnabled.Add(true);
  }
  for (int32 I = 0; I < Settings->Maps.Num(); ++I) {
    Model->MapNames.Add(Settings->Maps[I].Name.ToUpper());
    Model->MapEnabled.Add(I == CurrentMapIndex);
  }
  Model->ModeNames.Add(TEXT("FREE FLIGHT"));
  for (const FPlayModeEntry& Mode : Settings->Modes) Model->ModeNames.Add(Mode.Name.ToUpper());
  for (const FPlayVehicleEntry& Vehicle : Settings->Vehicles) Model->VehicleNames.Add(Vehicle.Name.ToUpper());
  for (const FPlayMoodEntry& Mood : Settings->Moods) Model->MoodNames.Add(Mood.Name.ToUpper());
  if (CurrentMapIndex != INDEX_NONE) LoadBackdrop(Settings->Maps[CurrentMapIndex]);
}

void AProjectAirSimPlayGameMode::LoadBackdrop(const FPlayMapEntry& Map) {
  if (Map.Image.IsEmpty()) return;
  const FString Path = FPaths::IsRelative(Map.Image) ? FPaths::Combine(FPaths::ProjectContentDir(), Map.Image) : Map.Image;
  TArray<double> Bounds;
  {
    TArray<FString> Parts;
    Map.ImageBoundsNed.ParseIntoArray(Parts, TEXT(","), true);
    if (Parts.Num() != 4) {
      UE_LOG(LogPlay, Fatal, TEXT("Map '%s' has an Image but ImageBoundsNed '%s' is not minX,minY,maxX,maxY"),
             *Map.Name, *Map.ImageBoundsNed);
    }
    for (const FString& Part : Parts) Bounds.Add(FCString::Atod(*Part));
  }
  TArray<uint8> File;
  if (!FFileHelper::LoadFileToArray(File, *Path)) {
    UE_LOG(LogPlay, Error, TEXT("Spawn-picker backdrop '%s' could not be read; drawing the grid only"), *Path);
    return;
  }
  IImageWrapperModule& Wrappers = FModuleManager::LoadModuleChecked<IImageWrapperModule>("ImageWrapper");
  TSharedPtr<IImageWrapper> Png = Wrappers.CreateImageWrapper(EImageFormat::PNG);
  TArray64<uint8> Raw;
  if (!Png || !Png->SetCompressed(File.GetData(), File.Num()) || !Png->GetRaw(ERGBFormat::BGRA, 8, Raw)) {
    UE_LOG(LogPlay, Error, TEXT("Spawn-picker backdrop '%s' is not a decodable PNG; drawing the grid only"), *Path);
    return;
  }
  TArray<uint8> Pixels(Raw.GetData(), Raw.Num());
  Model->Backdrop = FSlateDynamicImageBrush::CreateWithImageData(
      FName(*Path), FVector2f(Png->GetWidth(), Png->GetHeight()), Pixels);
  Model->BackdropNed = FBox2D(FVector2D(Bounds[0], Bounds[1]), FVector2D(Bounds[2], Bounds[3]));
}

void AProjectAirSimPlayGameMode::BuildActions() {
  Actions.NewFlight = [this]() { GoTo(EPlayScreen::Map); };
  Actions.Quit = [this]() {
    UE_LOG(LogPlay, Log, TEXT("Quit chosen from the title menu"));
    UKismetSystemLibrary::QuitGame(GetWorld(), GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
  };
  Actions.Back = [this]() {
    switch (Screen) {
      case EPlayScreen::Map: GoTo(EPlayScreen::Title); break;
      case EPlayScreen::Mode: GoTo(EPlayScreen::Map); break;
      case EPlayScreen::Mood: {
        const bool bFromPause = bMoodFromPause;
        bMoodFromPause = false;
        GoTo(bFromPause ? EPlayScreen::Pause : EPlayScreen::Mode);
        break;
      }
      case EPlayScreen::Spawn: GoTo(Settings->Moods.Num() > 0 ? EPlayScreen::Mood : EPlayScreen::Mode); break;
      case EPlayScreen::Vehicle: GoTo(EPlayScreen::Spawn); break;
      default: break;
    }
  };
  Actions.PickMap = [this](int32) { GoTo(EPlayScreen::Mode); };
  Actions.PickMode = [this](int32 Index) {
    SelectedMode = Index;
    bMoodFromPause = false;
    // The mood applies to free flight and the eval modes alike.
    if (Settings->Moods.Num() > 0) {
      GoTo(EPlayScreen::Mood);
      return;
    }
    ContinueFromMode();
  };
  Actions.PickMood = [this](int32 Index) {
    SelectMood(Index);
    if (bMoodFromPause) {
      bMoodFromPause = false;
      GoTo(EPlayScreen::Pause);
    } else {
      ContinueFromMode();
    }
  };
  Actions.OpenMood = [this]() {
    bMoodFromPause = true;
    GoTo(EPlayScreen::Mood);
  };
  Actions.PickSpawn = [this](const FVector3d& Ned) {
    bHasSpawn = true;
    SpawnNed = Ned;
    if (Settings->Vehicles.Num() > 0) {
      GoTo(EPlayScreen::Vehicle);
      return;
    }
    FPlayRunRequest Request;
    Request.Scenario = TEXT("free");
    Request.Env = Settings->Env;
    Request.bHasSpawn = true;
    Request.SpawnNed = SpawnNed;
    Launch(Request);
  };
  Actions.PickVehicle = [this](int32 Index) {
    FPlayRunRequest Request;
    Request.Scenario = TEXT("free");
    Request.Env = Settings->Env;
    Request.bHasSpawn = bHasSpawn;
    Request.SpawnNed = SpawnNed;
    Request.Vehicle = Settings->Vehicles[Index].RobotConfig;
    Launch(Request);
  };
  Actions.Resume = [this]() {
    InputMap.Reset();
    GoTo(Robot.IsValid() ? EPlayScreen::Flying : EPlayScreen::Loading);
  };
  Actions.Restart = [this]() { Launch(LastRequest); };
  Actions.BackToTitle = [this]() {
    StopRun();
    GoTo(EPlayScreen::Title);
  };
  Actions.Focused = [this]() { if (Audio) Audio->UiClick(); };
  Actions.Activated = [this]() { if (Audio) Audio->UiConfirm(); };
}

// ---------------------------------------------------------------------------
// Screens and requests

void AProjectAirSimPlayGameMode::ContinueFromMode() {
  if (SelectedMode == 0) {
    GoTo(EPlayScreen::Spawn);
    return;
  }
  // Eval modes keep their seeded spawn and the stock vehicle.
  const FPlayModeEntry& Mode = Settings->Modes[SelectedMode - 1];
  FPlayRunRequest Request;
  Request.Scenario = Mode.Scenario;
  Request.Env = Settings->Env;
  Request.Opts = ParsePlayOpts(Mode.Opts);
  Launch(Request);
}

void AProjectAirSimPlayGameMode::SelectMood(int32 Index) {
  check(MoodDirector);
  Model->CurrentMood = Index;
  MoodDirector->Apply(*GetWorld(), Index);
}

void AProjectAirSimPlayGameMode::GoTo(EPlayScreen Next) {
  UE_LOG(LogPlay, Log, TEXT("Screen %s -> %s"), ScreenName(Screen), ScreenName(Next));
  Screen = Next;
  if (Next == EPlayScreen::Spawn) GatherMarkers();
  const bool bUi = Next != EPlayScreen::Flying && Next != EPlayScreen::Loading;
  ApplyInputMode(bUi);
  Ui->ShowScreen(Next);

  // The mood list opened from the pause menu keeps the flight view and sound behind it.
  const bool bPauseMood = Next == EPlayScreen::Mood && bMoodFromPause;
  const bool bFlightScreen = Next == EPlayScreen::Flying || Next == EPlayScreen::Pause || Next == EPlayScreen::Loading ||
                             bPauseMood;
  if (Audio) {
    Audio->MenuAmbient(!bFlightScreen);
    Audio->FlightAmbient(bFlightScreen);
    if (!bFlightScreen) Audio->RotorLoop(-1.f);
  }
  if (Camera) {
    if (Next == EPlayScreen::Flying || Next == EPlayScreen::Pause || bPauseMood) {
      Camera->SetTarget(Robot.Get());
      Camera->SetMode(CameraIndex == 0 ? EPlayCameraMode::Chase : EPlayCameraMode::Fpv);
    } else {
      Camera->SetMode(EPlayCameraMode::Drift);
    }
  }
  if (Next != EPlayScreen::Flying) Shared->SetStick(FPlayStick());
}

void AProjectAirSimPlayGameMode::ApplyInputMode(bool bUi) {
  APlayerController* Controller = GetWorld()->GetFirstPlayerController();
  if (!Controller) {
    AppliedUiMode = -1;
    return;
  }
  Controller->bShowMouseCursor = bUi;
  if (bUi) {
    FInputModeUIOnly Mode;
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Controller->SetInputMode(Mode);
  } else {
    Controller->SetInputMode(FInputModeGameOnly());
  }
  AppliedUiMode = bUi ? 1 : 0;
}

void AProjectAirSimPlayGameMode::Launch(const FPlayRunRequest& Request) {
  LastRequest = Request;
  RobotBeforeLaunch = FindRobot();
  Robot.Reset();
  bHaveLastRobotLocation = false;
  PresetIndex = 1;
  CameraIndex = 0;
  TiltDegrees = 0.f;
  const int64 Seq = Shared->PostRun(Request);
  Model->HudReferee.Reset();
  Model->HudStats.Reset();
  LoadingBase = FString::Printf(TEXT("request %lld  %s"), Seq, *Request.Scenario);
  Model->LoadingNote = LoadingBase;
  LaunchWallSeconds = FPlatformTime::Seconds();
  UE_LOG(LogPlay, Log, TEXT("Run request %lld: scenario=%s vehicle=%s spawn=%d"), Seq, *Request.Scenario,
         *Request.Vehicle, Request.bHasSpawn);
  GoTo(EPlayScreen::Loading);
}

void AProjectAirSimPlayGameMode::StopRun() {
  const int64 Seq = Shared->PostStop();
  UE_LOG(LogPlay, Log, TEXT("Stop request %lld"), Seq);
  Robot.Reset();
  RobotBeforeLaunch.Reset();
  Model->HudReferee.Reset();
  Model->HudStats.Reset();
}

// ---------------------------------------------------------------------------
// Scene queries

AUnrealRobot* AProjectAirSimPlayGameMode::FindRobot() {
  if (Robot.IsValid()) return Robot.Get();
  const double Now = FPlatformTime::Seconds();
  if (Now < NextRobotSearch) return nullptr;
  NextRobotSearch = Now + 0.2;
  for (TActorIterator<AUnrealRobot> It(GetWorld()); It; ++It) {
    if (It->IsActorBeingDestroyed()) continue;
    return *It;
  }
  return nullptr;
}

void AProjectAirSimPlayGameMode::RemoveStockTabBinding(APlayerController& Controller) {
  const double Now = FPlatformTime::Seconds();
  if (Now < NextTabCheck || !Controller.PlayerInput) return;
  NextTabCheck = Now + 0.5;
  // The scene binds Tab to SwitchStreamingView on every load; here Tab cycles
  // the play camera.
  TArray<FInputActionKeyMapping> Stock;
  for (const FInputActionKeyMapping& Mapping : Controller.PlayerInput->ActionMappings) {
    if (Mapping.ActionName == FName("SwitchStreamingView")) Stock.Add(Mapping);
  }
  for (const FInputActionKeyMapping& Mapping : Stock) Controller.PlayerInput->RemoveActionMapping(Mapping);
}

void AProjectAirSimPlayGameMode::GatherMarkers() {
  Model->Markers.Reset();
  for (TActorIterator<ASpawnPoint> It(GetWorld()); It; ++It) {
    const FVector P = It->GetActorLocation();
    FPlaySpawnMarker Marker;
    Marker.Name = It->DisplayName.IsEmpty() ? It->GetName() : It->DisplayName;
    Marker.Ned = FVector3d(P.X / 100.0, P.Y / 100.0, -P.Z / 100.0);
    Model->Markers.Add(Marker);
  }
  for (const FPlaySpawnEntry& Spawn : Settings->Spawns) {
    const bool bThisMap = Spawn.Map == CurrentLevelName ||
                          (CurrentMapIndex != INDEX_NONE && Spawn.Map == Settings->Maps[CurrentMapIndex].Name);
    if (!bThisMap) continue;
    FPlaySpawnMarker Marker;
    Marker.Name = Spawn.Name;
    Marker.Ned = FVector3d(Spawn.X, Spawn.Y, Spawn.Z);
    Model->Markers.Add(Marker);
  }
}

void AProjectAirSimPlayGameMode::UpdateDriftCenter() {
  // The orbit centre is the middle of the known spawns, at ground level.
  const double Now = FPlatformTime::Seconds();
  if (Now < NextDriftRefresh || DriftRefreshes >= 20) return;
  NextDriftRefresh = Now + 3.0;
  ++DriftRefreshes;
  GatherMarkers();
  FVector2D Sum(0, 0);
  for (const FPlaySpawnMarker& M : Model->Markers) Sum += FVector2D(M.Ned.X * 100.0, M.Ned.Y * 100.0);
  const FVector2D Centre = Model->Markers.Num() ? Sum / Model->Markers.Num() : FVector2D::ZeroVector;
  FHitResult Hit;
  float Ground = 0.f;
  if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(Centre.X, Centre.Y, 300000.f),
                                           FVector(Centre.X, Centre.Y, -300000.f), ECC_Visibility)) {
    Ground = Hit.Location.Z;
  }
  Camera->SetDrift(FVector(Centre.X, Centre.Y, Ground), Settings->DriftRadiusM * 100.f,
                   Settings->DriftHeightM * 100.f);
}

// ---------------------------------------------------------------------------
// Per-frame

void AProjectAirSimPlayGameMode::Tick(float DeltaSeconds) {
  Super::Tick(DeltaSeconds);
  if (!Ui.IsValid()) return;

  const double WallNow = FPlatformTime::Seconds();
  const float WallDelta = static_cast<float>(FMath::Min(WallNow - LastWallSeconds, 0.25));
  LastWallSeconds = WallNow;

  APlayerController* Controller = GetWorld()->GetFirstPlayerController();
  if (!Controller) return;
  if (AppliedUiMode < 0) ApplyInputMode(Screen != EPlayScreen::Flying && Screen != EPlayScreen::Loading);

  RemoveStockTabBinding(*Controller);
  if (Screen == EPlayScreen::Intro || Screen == EPlayScreen::Title) UpdateDriftCenter();
  UpdateLaunchSequence();

  AUnrealRobot* Found = FindRobot();
  if (Found && Found != RobotBeforeLaunch.Get()) Robot = Found;
  if (Screen == EPlayScreen::Loading && Robot.IsValid()) {
    GoTo(EPlayScreen::Flying);
  } else if (Screen == EPlayScreen::Flying && !Robot.IsValid()) {
    // The attendant tore the scene down; wait for the next robot. A paused
    // player stays on the pause menu; Resume returns to loading if needed.
    RobotBeforeLaunch.Reset();
    GoTo(EPlayScreen::Loading);
  }

  if (Screen == EPlayScreen::Flying || Screen == EPlayScreen::Loading) {
    TickFlight(*Controller, WallDelta);
  }

  // The play camera is always the view target.
  if (Controller->GetViewTarget() != Camera) Controller->SetViewTarget(Camera);
  Camera->SetTilt(TiltDegrees);
  Camera->Advance(WallDelta);

  if (MoodDirector) MoodDirector->Tick(*GetWorld());
  UpdateHud(WallDelta);
  UpdateFootnotes();
  MuteProjectSounds();
  LogFrameRate();
  Ui->ApplyPendingFocus();
  if (TestScript) TestScript->Tick(TestHooks);
}

void AProjectAirSimPlayGameMode::TickFlight(APlayerController& Controller, float WallDelta) {
  const FPlayInputSample In = InputMap.Poll(Controller);
  if (In.Pressed(EPlayButton::Pause)) {
    GoTo(EPlayScreen::Pause);
    return;
  }
  if (Screen != EPlayScreen::Flying) return;

  // A run that ended with a result: Enter goes back to the title, R runs it again.
  if (Model->HudRefereeLevel == TEXT("result")) {
    if (In.Pressed(EPlayButton::Confirm)) {
      Actions.BackToTitle();
      return;
    }
    if (In.Pressed(EPlayButton::Restart)) {
      Actions.Restart();
      return;
    }
  }

  if (In.Pressed(EPlayButton::PresetDown)) PresetIndex = FMath::Max(PresetIndex - 1, 0);
  if (In.Pressed(EPlayButton::PresetUp)) PresetIndex = FMath::Min(PresetIndex + 1, kPresetCount - 1);
  if (In.Pressed(EPlayButton::CycleCamera)) {
    CameraIndex = (CameraIndex + 1) % 2;
    Camera->SetMode(CameraIndex == 0 ? EPlayCameraMode::Chase : EPlayCameraMode::Fpv);
  }
  if (In.Pressed(EPlayButton::TakeoffLand)) Shared->PressTakeoffLand();
  TiltDegrees = FMath::Clamp(TiltDegrees + In.Axis(EPlayAxis::CameraTilt) * kTiltRateDegPerSec * WallDelta,
                             kTiltMinDeg, kTiltMaxDeg);

  if (bStickOverride) {
    FPlayStick Held = OverrideStick;
    Held.Camera = CameraIndex;
    Shared->SetStick(Held);
    return;
  }
  // Body frame, NED: x forward, y right, z down.
  const float Speed = kPresetSpeeds[PresetIndex];
  FPlayStick Stick;
  Stick.Vx = In.Axis(EPlayAxis::Forward) * Speed;
  Stick.Vy = In.Axis(EPlayAxis::Right) * Speed;
  Stick.Vz = -In.Axis(EPlayAxis::Climb) * Speed * 0.5f;
  Stick.YawRate = In.Axis(EPlayAxis::Yaw) * kYawRate;
  Stick.Camera = CameraIndex;
  Shared->SetStick(Stick);
}

void AProjectAirSimPlayGameMode::UpdateHud(float WallDelta) {
  Shared->GetHud(Model->HudReferee, Model->HudRefereeLevel);
  if (Model->HudRefereeLevel == TEXT("result")) Model->HudReferee += TEXT("    ENTER: TITLE   R: RESTART");
  AUnrealRobot* Target = Robot.Get();
  if (!Target) return;

  const FVector Location = Target->GetActorLocation();
  if (bHaveLastRobotLocation && WallDelta > 0.f) {
    const float Instant = FVector::Dist(Location, LastRobotLocation) / 100.f / WallDelta;
    SpeedMs = FMath::Lerp(SpeedMs, Instant, FMath::Min(1.f, WallDelta * 6.f));
  }
  LastRobotLocation = Location;
  bHaveLastRobotLocation = true;

  FHitResult Hit;
  FCollisionQueryParams Params(SCENE_QUERY_STAT(PlayAgl), false, Target);
  AltitudeM = GetWorld()->LineTraceSingleByChannel(Hit, Location, Location - FVector(0, 0, 100000.f), ECC_Visibility,
                                                    Params)
                  ? (Location.Z - Hit.Location.Z) / 100.f
                  : Location.Z / 100.f;
  Model->HudStats = FString::Printf(TEXT("ALT %6.1f m   SPD %5.1f m/s   PRESET %2.0f m/s   CAM %s"), AltitudeM,
                                    SpeedMs, kPresetSpeeds[PresetIndex], CameraIndex == 0 ? TEXT("CHASE") : TEXT("FPV"));
  // No thrust is published to the plugin side; the measured speed stands in for throttle.
  if (Audio && Screen == EPlayScreen::Flying) {
    Audio->RotorLoop(FMath::Clamp(0.25f + SpeedMs / 25.f, 0.f, 1.f));
  }
}

void AProjectAirSimPlayGameMode::UpdateLaunchSequence() {
  const double Now = FPlatformTime::Seconds();
  if (Screen == EPlayScreen::Intro) {
    const double T = Now - IntroStartSeconds;
    Model->LogoProgress = static_cast<float>(FMath::Clamp(T / kIntroBuild, 0.0, 1.0));
    Model->LogoAlpha = static_cast<float>(
        1.0 - FMath::Clamp((T - kIntroBuild - kIntroHold) / kIntroFade, 0.0, 1.0));
    if (T < kIntroBuild + kIntroHold + kIntroFade) return;
    Model->LogoAlpha = 0.f;
    CurtainLiftSeconds = Now;
    GoTo(EPlayScreen::Title);
  }
  if (CurtainLiftSeconds >= 0.0) {
    const double Lifted = FMath::Clamp((Now - CurtainLiftSeconds) / kCurtainLift, 0.0, 1.0);
    Model->CurtainAlpha = static_cast<float>(1.0 - Lifted);
    if (Lifted >= 1.0) CurtainLiftSeconds = -1.0;
  }
}

void AProjectAirSimPlayGameMode::UpdateFootnotes() {
  const double Since = Shared->SecondsSinceAttendantPoll();
  if (Since < 0.0) {
    Model->Footnote = TEXT("NO ATTENDANT - START WITH run.sh --play TO FLY");
  } else if (Since < 3.0) {
    Model->Footnote = TEXT("ATTENDANT CONNECTED");
  } else {
    Model->Footnote = FString::Printf(TEXT("ATTENDANT SILENT FOR %.0f s"), Since);
  }
  if (Screen == EPlayScreen::Loading) {
    const double Waited = FPlatformTime::Seconds() - LaunchWallSeconds;
    FString Note = FString::Printf(TEXT("%s  (%.0f s)"), *LoadingBase, Waited);
    if (Since < 0.0 || Since > 3.0) Note += TEXT("  - attendant not answering");
    Model->LoadingNote = Note;
  }
}

// ---------------------------------------------------------------------------
// Test hooks (see PlayTestScript.h)

const TCHAR* AProjectAirSimPlayGameMode::ScreenName(EPlayScreen Screen) {
  switch (Screen) {
    case EPlayScreen::Intro: return TEXT("intro");
    case EPlayScreen::Title: return TEXT("title");
    case EPlayScreen::Map: return TEXT("map");
    case EPlayScreen::Mode: return TEXT("mode");
    case EPlayScreen::Mood: return TEXT("mood");
    case EPlayScreen::Spawn: return TEXT("spawn");
    case EPlayScreen::Vehicle: return TEXT("vehicle");
    case EPlayScreen::Pause: return TEXT("pause");
    case EPlayScreen::Loading: return TEXT("loading");
    case EPlayScreen::Flying: return TEXT("flying");
  }
  UE_LOG(LogPlay, Fatal, TEXT("Unhandled play screen"));
  return TEXT("");
}

void AProjectAirSimPlayGameMode::BuildTestHooks() {
  TestHooks.Key = [this](const FKey& Key, bool bDown) {
    const bool bGameInput = Screen == EPlayScreen::Flying || Screen == EPlayScreen::Loading;
    if (bGameInput) {
      APlayerController* Controller = GetWorld()->GetFirstPlayerController();
      check(Controller);
      Controller->InputKey(FInputKeyParams(Key, bDown ? IE_Pressed : IE_Released, 1.0, false));
      return;
    }
    const uint32* KeyCode = nullptr;
    const uint32* CharCode = nullptr;
    FInputKeyManager::Get().GetCodesFromKey(Key, KeyCode, CharCode);
    const FKeyEvent Event(Key, FModifierKeysState(), 0, false, CharCode ? *CharCode : 0, KeyCode ? *KeyCode : 0);
    if (bDown) {
      FSlateApplication::Get().ProcessKeyDownEvent(Event);
    } else {
      FSlateApplication::Get().ProcessKeyUpEvent(Event);
    }
  };
  TestHooks.Screenshot = [](const FString& Path) {
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
    FScreenshotRequest::RequestScreenshot(Path, true, false);
  };
  TestHooks.Stick = [this](const FPlayStick& Stick) {
    bStickOverride = true;
    OverrideStick = Stick;
  };
  TestHooks.ClearStick = [this]() {
    bStickOverride = false;
    Shared->SetStick(FPlayStick());
  };
  TestHooks.TakeoffLand = [this]() { Shared->PressTakeoffLand(); };
  TestHooks.ScreenName = [this]() { return FString(ScreenName(Screen)); };
  TestHooks.HudText = [this]() { return Model->HudStats + TEXT(" | ") + Model->HudReferee; };
  TestHooks.Quit = []() { FPlatformMisc::RequestExit(false); };
}

// ---------------------------------------------------------------------------
// Project sounds and frame rate

void AProjectAirSimPlayGameMode::MuteProjectSounds() {
  if (Settings->MutedSounds.Num() == 0) return;
  const double Now = FPlatformTime::Seconds();
  if (Now < NextMuteSweep) return;
  NextMuteSweep = Now + 1.0;
  for (TObjectIterator<UAudioComponent> It; It; ++It) {
    UAudioComponent* Component = *It;
    if (!Component->IsPlaying() || Component->GetWorld() != GetWorld()) continue;
    // The play mode's own procedural audio streams through an audio component owned by UPlayAudio.
    if (Component->GetTypedOuter<UPlayAudio>()) continue;
    const USoundBase* Sound = Component->Sound;
    if (!Sound) continue;
    const FString Name = Sound->GetName();
    for (const FString& Muted : Settings->MutedSounds) {
      if (Name.Contains(Muted)) {
        UE_LOG(LogPlay, Log, TEXT("Muting project sound %s"), *Name);
        Component->Stop();
        break;
      }
    }
  }
}

void AProjectAirSimPlayGameMode::LogFrameRate() {
  const double Now = FPlatformTime::Seconds();
  ++FpsWindowFrames;
  if (FpsWindowStart == 0.0) FpsWindowStart = Now;
  if (Now - FpsWindowStart < 10.0) return;
  UE_LOG(LogPlay, Log, TEXT("Frame rate %.1f fps over %.1f s (screen %s)"), FpsWindowFrames / (Now - FpsWindowStart),
         Now - FpsWindowStart, ScreenName(Screen));
  FpsWindowStart = Now;
  FpsWindowFrames = 0;
}
