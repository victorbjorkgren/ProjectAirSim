// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Play/PlayLights.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"

enum class EPlayScreen : uint8 { Intro, Title, Map, Mode, Mood, Spawn, Vehicle, Pause, Lights, Loading, Flying };

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
  TArray<FString> MoodNames;  // empty: the build has no moods and the mood entries do not appear
  int32 CurrentMood = INDEX_NONE;
  // The drone lights the LIGHTS panel edits, and the presets it cycles through.
  TSharedPtr<FPlayLightsState> Lights;
  const UPlaySettings* LightSettings = nullptr;
  TArray<FPlaySpawnMarker> Markers;
  TSharedPtr<FSlateBrush> Backdrop;
  FBox2D BackdropNed = FBox2D(ForceInit);  // x = north, y = east
  FString Footnote;     // title screen: attendant status
  FString LoadingNote;  // loading screen detail
  FString HudStats;
  FString HudReferee;
  FString HudRefereeLevel = TEXT("info");
  // Launch sequence: the mark's build-up progress (0..1), its opacity, and the
  // black curtain over everything else (1 = fully black).
  float LogoProgress = 0.f;
  float LogoAlpha = 0.f;
  float CurtainAlpha = 0.f;
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
  TFunction<void(int32)> PickMood;
  TFunction<void()> OpenMood;  // the pause menu's mood entry
  TFunction<void()> OpenLights;  // the pause menu's lights entry
  TFunction<void()> LightsFlashlight;
  TFunction<void()> LightsNav;
  TFunction<void()> LightsMode;
  TFunction<void()> LightsPreset;
  TFunction<void()> LightsTail;
  TFunction<void()> LightsEdited;  // the panel changed a colour in the shared state
  TFunction<void(const FVector3d&)> PickSpawn;
  TFunction<void()> Focused;    // a menu item took focus
  TFunction<void()> Activated;  // a menu item was chosen
};

// The Nordic tri-chevron mark building itself up, drawn as lines in the
// 48 x 48 view box of the app mark and scaled to the widget.
class SPlayLogo : public SLeafWidget {
 public:
  SLATE_BEGIN_ARGS(SPlayLogo) {}
  SLATE_ATTRIBUTE(float, Progress)
  SLATE_ATTRIBUTE(float, Alpha)
  SLATE_END_ARGS()

  void Construct(const FArguments& InArgs);

  FVector2D ComputeDesiredSize(float) const override { return FVector2D(140, 140); }
  int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
                FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style,
                bool bParentEnabled) const override;

 private:
  TAttribute<float> Progress;
  TAttribute<float> Alpha;
};

// A menu item: plain text, white while hovered or focused, grey otherwise.
class SPlayMenuItem : public SCompoundWidget {
 public:
  SLATE_BEGIN_ARGS(SPlayMenuItem) : _FontSize(22), _Enabled(true) {}
  SLATE_ARGUMENT(FText, Label)
  // When set, the label is read every frame (a toggle that shows its state).
  SLATE_ARGUMENT(TFunction<FString()>, LabelFn)
  SLATE_ARGUMENT(int32, FontSize)
  SLATE_ARGUMENT(bool, Enabled)
  SLATE_EVENT(FSimpleDelegate, OnActivated)
  SLATE_EVENT(FSimpleDelegate, OnFocused)
  SLATE_END_ARGS()

  void Construct(const FArguments& InArgs);

  bool SupportsKeyboardFocus() const override { return bItemEnabled; }
  // The item shows its own selection; no focus rectangle on top.
  TOptional<bool> OnQueryShowFocus(const EFocusCause) const override { return false; }
  FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;
  FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;
  void OnMouseEnter(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;
  FReply OnFocusReceived(const FGeometry& Geometry, const FFocusEvent& FocusEvent) override;

 private:
  bool bItemEnabled = true;
  FSimpleDelegate Activated;
  FSimpleDelegate FocusedDelegate;
};

// A horizontal 0..255 track: a dim bar, the filled part in the tint colour, a square handle. The mouse drags it.
class SPlaySliderBar : public SLeafWidget {
 public:
  SLATE_BEGIN_ARGS(SPlaySliderBar) {}
  SLATE_ARGUMENT(FLinearColor, Tint)
  SLATE_ATTRIBUTE(int32, Value)
  SLATE_EVENT(TDelegate<void(int32)>, OnChanged)
  SLATE_END_ARGS()

  void Construct(const FArguments& InArgs);

  FVector2D ComputeDesiredSize(float) const override { return FVector2D(200, 20); }
  int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
                FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style,
                bool bParentEnabled) const override;
  FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;
  FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;
  FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;

 private:
  void SetFromMouse(const FGeometry& Geometry, const FPointerEvent& MouseEvent);

  FLinearColor Tint = FLinearColor::White;
  TAttribute<int32> Value;
  TDelegate<void(int32)> Changed;
  bool bDragging = false;
};

// One colour channel of the LIGHTS panel: a label, a slider and the 0..255 value. Left/Right keys step the value
// (Shift: one unit), the mouse drags the slider.
class SPlayChannelSlider : public SCompoundWidget {
 public:
  SLATE_BEGIN_ARGS(SPlayChannelSlider) {}
  SLATE_ARGUMENT(FText, Label)
  SLATE_ARGUMENT(FLinearColor, Tint)
  SLATE_ATTRIBUTE(int32, Value)
  SLATE_EVENT(TDelegate<void(int32)>, OnChanged)
  SLATE_EVENT(FSimpleDelegate, OnFocused)
  SLATE_END_ARGS()

  void Construct(const FArguments& InArgs);

  bool SupportsKeyboardFocus() const override { return true; }
  TOptional<bool> OnQueryShowFocus(const EFocusCause) const override { return false; }
  FReply OnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;
  void OnMouseEnter(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;
  FReply OnFocusReceived(const FGeometry& Geometry, const FFocusEvent& FocusEvent) override;

 private:
  TAttribute<int32> Value;
  TDelegate<void(int32)> Changed;
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
  TOptional<bool> OnQueryShowFocus(const EFocusCause) const override { return false; }
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
  TSharedRef<SWidget> MakeDynamicItem(TFunction<FString()> Label, int32 FontSize, TFunction<void()> Action);
  TSharedRef<SWidget> BuildLights();
  TSharedRef<SWidget> MakePanel(const FString& Heading, TSharedRef<SWidget> Body);
  TSharedRef<SWidget> MakeCurtain();
  TSharedRef<SWidget> MakeHud();

  TSharedPtr<FPlayUiModel> Model;
  FPlayUiActions Actions;
  EPlayScreen Current = EPlayScreen::Intro;
  TSharedPtr<class SBox> ScreenHolder;
  TSharedPtr<class SWidget> HudWidget;
  TWeakPtr<SWidget> FirstFocus;
  bool bPendingFocus = false;
};
