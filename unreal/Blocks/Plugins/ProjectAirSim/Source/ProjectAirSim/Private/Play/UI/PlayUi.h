// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"

enum class EPlayScreen : uint8 { Title, Map, Mode, Spawn, Vehicle, Pause, Loading, Flying };

struct FPlaySpawnMarker {
  FString Name;
  FVector3d Ned = FVector3d::ZeroVector;  // metres
};

// What the screens show. The game mode owns and updates it.
struct FPlayUiModel {
  FString Title;
  TArray<FString> MapNames;
  TArray<bool> MapEnabled;
  TArray<FString> ModeNames;  // index 0 is free flight
  TArray<FString> VehicleNames;
  TArray<FPlaySpawnMarker> Markers;
  TSharedPtr<FSlateBrush> Backdrop;
  FBox2D BackdropNed = FBox2D(ForceInit);  // x = north, y = east
  FString Footnote;     // title screen: attendant status
  FString LoadingNote;  // loading screen detail
  FString HudStats;
  FString HudReferee;
  FString HudRefereeLevel = TEXT("info");
};

// What the screens ask for.
struct FPlayUiActions {
  TFunction<void()> NewFlight;
  TFunction<void()> Quit;
  TFunction<void()> Back;
  TFunction<void()> Resume;
  TFunction<void()> Restart;
  TFunction<void()> BackToTitle;
  TFunction<void(int32)> PickMap;
  TFunction<void(int32)> PickMode;
  TFunction<void(int32)> PickVehicle;
  TFunction<void(const FVector3d&)> PickSpawn;
  TFunction<void()> Focused;    // a menu item took focus
  TFunction<void()> Activated;  // a menu item was chosen
};

// A menu item: plain text, lit while hovered or focused.
class SPlayMenuItem : public SCompoundWidget {
 public:
  SLATE_BEGIN_ARGS(SPlayMenuItem) : _Enabled(true) {}
  SLATE_ARGUMENT(FText, Label)
  SLATE_ARGUMENT(bool, Enabled)
  SLATE_EVENT(FSimpleDelegate, OnActivated)
  SLATE_EVENT(FSimpleDelegate, OnFocused)
  SLATE_END_ARGS()

  void Construct(const FArguments& InArgs);

  bool SupportsKeyboardFocus() const override { return bItemEnabled; }
  FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;
  FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;
  void OnMouseEnter(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;
  FReply OnFocusReceived(const FGeometry& Geometry, const FFocusEvent& FocusEvent) override;

 private:
  bool bItemEnabled = true;
  FSimpleDelegate Activated;
  FSimpleDelegate FocusedDelegate;
};

// Tactical top-down spawn picker: dark ground, a thin NED grid with labels,
// a marker per spawn, and an optional backdrop image.
class SPlaySpawnPicker : public SLeafWidget {
 public:
  SLATE_BEGIN_ARGS(SPlaySpawnPicker) {}
  SLATE_ARGUMENT(TArray<FPlaySpawnMarker>, Markers)
  SLATE_ARGUMENT(TSharedPtr<FSlateBrush>, Backdrop)
  SLATE_ARGUMENT(FBox2D, BackdropNed)
  SLATE_EVENT(TDelegate<void(const FVector3d&)>, OnPicked)
  SLATE_EVENT(FSimpleDelegate, OnCancel)
  SLATE_END_ARGS()

  void Construct(const FArguments& InArgs);

  bool SupportsKeyboardFocus() const override { return true; }
  FVector2D ComputeDesiredSize(float) const override { return FVector2D(640, 480); }
  int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
                FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style,
                bool bParentEnabled) const override;
  FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;
  FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;
  FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;
  FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;
  FReply OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;
  void OnMouseLeave(const FPointerEvent& MouseEvent) override;

 private:
  FVector2f NedToLocal(const FVector2d& Ned, const FGeometry& G) const;
  FVector2d LocalToNed(const FVector2f& Local, const FGeometry& G) const;
  int32 MarkerAt(const FVector2f& Local, const FGeometry& G) const;
  void FitView(const FVector2f& Size) const;

  TArray<FPlaySpawnMarker> Markers;
  TSharedPtr<FSlateBrush> Backdrop;
  FBox2D BackdropNed = FBox2D(ForceInit);
  TDelegate<void(const FVector3d&)> Picked;
  FSimpleDelegate Cancelled;

  // View: NED centre (north, east) and pixels per metre. Fitted on first paint.
  mutable FVector2d Center = FVector2d::ZeroVector;
  mutable double PixelsPerMetre = 1.0;
  mutable bool bFitted = false;
  int32 Selected = INDEX_NONE;
  int32 Hovered = INDEX_NONE;
  bool bDragging = false;
  bool bDragMoved = false;
  FVector2f DragLast = FVector2f::ZeroVector;
  FVector2f HoverLocal = FVector2f::ZeroVector;
  bool bMouseInside = false;
};

// Owns every screen and the HUD. The game mode picks the screen; the widget
// asks for transitions through the actions.
class SPlayUi : public SCompoundWidget {
 public:
  SLATE_BEGIN_ARGS(SPlayUi) {}
  SLATE_ARGUMENT(TSharedPtr<FPlayUiModel>, Model)
  SLATE_ARGUMENT(FPlayUiActions, Actions)
  SLATE_END_ARGS()

  void Construct(const FArguments& InArgs);

  void ShowScreen(EPlayScreen Screen);
  EPlayScreen GetScreen() const { return Current; }
  // Gives the first item of the screen keyboard focus once it is on screen.
  void ApplyPendingFocus();

  FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

 private:
  TSharedRef<SWidget> BuildScreen(EPlayScreen Screen);
  TSharedRef<SWidget> MakeItem(const FString& Label, bool bEnabled, TFunction<void()> Action);
  TSharedRef<SWidget> MakePanel(const FString& Heading, TSharedRef<SWidget> Body);
  TSharedRef<SWidget> MakeHud();

  TSharedPtr<FPlayUiModel> Model;
  FPlayUiActions Actions;
  EPlayScreen Current = EPlayScreen::Title;
  TSharedPtr<class SBox> ScreenHolder;
  TSharedPtr<class SWidget> HudWidget;
  TWeakPtr<SWidget> FirstFocus;
  bool bPendingFocus = false;
};
