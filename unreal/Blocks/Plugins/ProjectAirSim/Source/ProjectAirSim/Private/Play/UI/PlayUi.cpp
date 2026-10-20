// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#include "Play/UI/PlayUi.h"

#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace {
const FLinearColor kInk(0.88f, 0.89f, 0.90f, 1.f);
const FLinearColor kDim(0.88f, 0.89f, 0.90f, 0.5f);
const FLinearColor kAccent(1.00f, 0.74f, 0.25f, 1.f);
const FLinearColor kWarn(1.00f, 0.55f, 0.20f, 1.f);
const FLinearColor kResult(0.55f, 0.90f, 0.60f, 1.f);

FSlateFontInfo Font(const char* Face, int32 Size) {
  return FCoreStyle::GetDefaultFontStyle(Face, Size);
}

const FSlateBrush* White() { return FCoreStyle::Get().GetBrush("WhiteBrush"); }
}  // namespace

// ---------------------------------------------------------------------------
// SPlayMenuItem

void SPlayMenuItem::Construct(const FArguments& InArgs) {
  bItemEnabled = InArgs._Enabled;
  Activated = InArgs._OnActivated;
  FocusedDelegate = InArgs._OnFocused;
  const FText Label = InArgs._Label;
  ChildSlot[SNew(SBox).Padding(FMargin(0, 5))[
      SNew(STextBlock)
          .Text_Lambda([this, Label]() {
            return (HasKeyboardFocus() || IsHovered()) && bItemEnabled
                       ? FText::FromString(TEXT("- ") + Label.ToString())
                       : Label;
          })
          .Font(Font("Regular", 22))
          .ColorAndOpacity_Lambda([this]() {
            if (!bItemEnabled) return FSlateColor(FLinearColor(1, 1, 1, 0.22f));
            return FSlateColor((HasKeyboardFocus() || IsHovered()) ? kAccent : kDim);
          })]];
}

FReply SPlayMenuItem::OnKeyDown(const FGeometry&, const FKeyEvent& KeyEvent) {
  const FKey Key = KeyEvent.GetKey();
  if (bItemEnabled && (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Virtual_Accept)) {
    Activated.ExecuteIfBound();
    return FReply::Handled();
  }
  return FReply::Unhandled();
}

FReply SPlayMenuItem::OnMouseButtonUp(const FGeometry&, const FPointerEvent& MouseEvent) {
  if (bItemEnabled && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) {
    Activated.ExecuteIfBound();
    return FReply::Handled();
  }
  return FReply::Unhandled();
}

void SPlayMenuItem::OnMouseEnter(const FGeometry& Geometry, const FPointerEvent& MouseEvent) {
  SCompoundWidget::OnMouseEnter(Geometry, MouseEvent);
  if (bItemEnabled) {
    FSlateApplication::Get().SetKeyboardFocus(AsShared(), EFocusCause::Mouse);
  }
}

FReply SPlayMenuItem::OnFocusReceived(const FGeometry&, const FFocusEvent&) {
  FocusedDelegate.ExecuteIfBound();
  return FReply::Handled();
}

// ---------------------------------------------------------------------------
// SPlaySpawnPicker

void SPlaySpawnPicker::Construct(const FArguments& InArgs) {
  Markers = InArgs._Markers;
  Backdrop = InArgs._Backdrop;
  BackdropNed = InArgs._BackdropNed;
  Picked = InArgs._OnPicked;
  Cancelled = InArgs._OnCancel;
  Selected = Markers.Num() > 0 ? 0 : INDEX_NONE;
}

void SPlaySpawnPicker::FitView(const FVector2f& Size) const {
  FBox2D Bounds(ForceInit);
  for (const FPlaySpawnMarker& M : Markers) Bounds += FVector2D(M.Ned.X, M.Ned.Y);
  if (Backdrop.IsValid() && BackdropNed.bIsValid) {
    Bounds += BackdropNed.Min;
    Bounds += BackdropNed.Max;
  }
  if (!Bounds.bIsValid) Bounds = FBox2D(FVector2D(-100, -100), FVector2D(100, 100));
  Center = Bounds.GetCenter();
  const FVector2D Extent = Bounds.GetSize() * 0.5;
  const double HalfNorth = FMath::Max(Extent.X, 100.0);
  const double HalfEast = FMath::Max(Extent.Y, 100.0);
  // Leave a margin around the outermost marker.
  PixelsPerMetre = FMath::Min(Size.Y * 0.40 / HalfNorth, Size.X * 0.40 / HalfEast);
  bFitted = true;
}

FVector2f SPlaySpawnPicker::NedToLocal(const FVector2d& Ned, const FGeometry& G) const {
  const FVector2f Size = G.GetLocalSize();
  return FVector2f(Size.X * 0.5f + (Ned.Y - Center.Y) * PixelsPerMetre,
                   Size.Y * 0.5f - (Ned.X - Center.X) * PixelsPerMetre);
}

FVector2d SPlaySpawnPicker::LocalToNed(const FVector2f& Local, const FGeometry& G) const {
  const FVector2f Size = G.GetLocalSize();
  return FVector2d(Center.X - (Local.Y - Size.Y * 0.5f) / PixelsPerMetre,
                   Center.Y + (Local.X - Size.X * 0.5f) / PixelsPerMetre);
}

int32 SPlaySpawnPicker::MarkerAt(const FVector2f& Local, const FGeometry& G) const {
  int32 Best = INDEX_NONE;
  float BestDist = 16.f;
  for (int32 I = 0; I < Markers.Num(); ++I) {
    const float D = FVector2f::Distance(Local, NedToLocal(FVector2d(Markers[I].Ned.X, Markers[I].Ned.Y), G));
    if (D < BestDist) {
      BestDist = D;
      Best = I;
    }
  }
  return Best;
}

int32 SPlaySpawnPicker::OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&,
                                FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle&,
                                bool) const {
  const FVector2f Size = G.GetLocalSize();
  if (!bFitted) FitView(Size);

  FSlateDrawElement::MakeBox(Out, LayerId, G.ToPaintGeometry(), White(), ESlateDrawEffect::None,
                             FLinearColor(0.035f, 0.045f, 0.055f, 0.96f));

  if (Backdrop.IsValid() && BackdropNed.bIsValid) {
    const FVector2f TopLeft = NedToLocal(FVector2d(BackdropNed.Max.X, BackdropNed.Min.Y), G);
    const FVector2f Extent(BackdropNed.GetSize().Y * PixelsPerMetre, BackdropNed.GetSize().X * PixelsPerMetre);
    FSlateDrawElement::MakeBox(Out, LayerId + 1, G.ToPaintGeometry(Extent, FSlateLayoutTransform(TopLeft)),
                               Backdrop.Get(), ESlateDrawEffect::None, FLinearColor(1, 1, 1, 0.85f));
  }

  // Grid: the smallest step from 1-2-5 whose lines stay at least 70 px apart.
  const double Mantissa[3] = {1.0, 2.0, 5.0};
  int32 MantissaIndex = 0;
  double Decade = 1.0;
  double Step = 1.0;
  while (Step * PixelsPerMetre < 70.0) {
    if (++MantissaIndex == 3) {
      MantissaIndex = 0;
      Decade *= 10.0;
    }
    Step = Mantissa[MantissaIndex] * Decade;
  }
  const FVector2d TopLeftNed = LocalToNed(FVector2f(0, 0), G);
  const FVector2d BottomRightNed = LocalToNed(Size, G);
  const FLinearColor GridColor(0.45f, 0.55f, 0.62f, 0.16f);
  const FLinearColor LabelColor(0.60f, 0.70f, 0.78f, 0.55f);
  for (double North = FMath::CeilToDouble(BottomRightNed.X / Step) * Step; North <= TopLeftNed.X; North += Step) {
    const float Y = NedToLocal(FVector2d(North, Center.Y), G).Y;
    FSlateDrawElement::MakeLines(Out, LayerId + 2, G.ToPaintGeometry(),
                                 TArray<FVector2f>{FVector2f(0, Y), FVector2f(Size.X, Y)},
                                 ESlateDrawEffect::None, GridColor, false, 1.f);
    FSlateDrawElement::MakeText(Out, LayerId + 3, G.ToPaintGeometry(FSlateLayoutTransform(FVector2f(8, Y - 14))),
                                FString::Printf(TEXT("N %g"), North), Font("Mono", 9), ESlateDrawEffect::None,
                                LabelColor);
  }
  for (double East = FMath::CeilToDouble(TopLeftNed.Y / Step) * Step; East <= BottomRightNed.Y; East += Step) {
    const float X = NedToLocal(FVector2d(Center.X, East), G).X;
    FSlateDrawElement::MakeLines(Out, LayerId + 2, G.ToPaintGeometry(),
                                 TArray<FVector2f>{FVector2f(X, 0), FVector2f(X, Size.Y)},
                                 ESlateDrawEffect::None, GridColor, false, 1.f);
    FSlateDrawElement::MakeText(Out, LayerId + 3,
                                G.ToPaintGeometry(FSlateLayoutTransform(FVector2f(X + 4, Size.Y - 22))),
                                FString::Printf(TEXT("E %g"), East), Font("Mono", 9), ESlateDrawEffect::None,
                                LabelColor);
  }

  const int32 HoverNow = bMouseInside ? MarkerAt(HoverLocal, G) : INDEX_NONE;
  for (int32 I = 0; I < Markers.Num(); ++I) {
    const FVector2f P = NedToLocal(FVector2d(Markers[I].Ned.X, Markers[I].Ned.Y), G);
    const bool bSel = I == Selected;
    const bool bHot = I == HoverNow;
    const float R = bSel || bHot ? 9.f : 6.f;
    const FLinearColor C = bSel || bHot ? kAccent : FLinearColor(0.75f, 0.85f, 0.92f, 0.9f);
    FSlateDrawElement::MakeLines(
        Out, LayerId + 4, G.ToPaintGeometry(),
        TArray<FVector2f>{P + FVector2f(0, -R), P + FVector2f(R, 0), P + FVector2f(0, R),
                          P + FVector2f(-R, 0), P + FVector2f(0, -R)},
        ESlateDrawEffect::None, C, true, bSel ? 2.f : 1.5f);
    if (bSel) {
      FSlateDrawElement::MakeLines(Out, LayerId + 4, G.ToPaintGeometry(),
                                   TArray<FVector2f>{P + FVector2f(-R - 5, 0), P + FVector2f(-R - 14, 0)},
                                   ESlateDrawEffect::None, C, true, 1.5f);
      FSlateDrawElement::MakeLines(Out, LayerId + 4, G.ToPaintGeometry(),
                                   TArray<FVector2f>{P + FVector2f(R + 5, 0), P + FVector2f(R + 14, 0)},
                                   ESlateDrawEffect::None, C, true, 1.5f);
    }
    FSlateDrawElement::MakeText(Out, LayerId + 5, G.ToPaintGeometry(FSlateLayoutTransform(P + FVector2f(R + 8, -9))),
                                Markers[I].Name, Font("Regular", 12), ESlateDrawEffect::None, C);
  }
  return LayerId + 6;
}

FReply SPlaySpawnPicker::OnKeyDown(const FGeometry&, const FKeyEvent& KeyEvent) {
  const FKey Key = KeyEvent.GetKey();
  const int32 N = Markers.Num();
  if (Key == EKeys::Escape) {
    Cancelled.ExecuteIfBound();
    return FReply::Handled();
  }
  if (N > 0) {
    const bool bBack = Key == EKeys::Left || Key == EKeys::Up || (Key == EKeys::Tab && KeyEvent.IsShiftDown());
    const bool bForward = Key == EKeys::Right || Key == EKeys::Down || (Key == EKeys::Tab && !KeyEvent.IsShiftDown());
    if (bForward || bBack) {
      Selected = (Selected + (bForward ? 1 : N - 1)) % N;
      return FReply::Handled();
    }
  }
  if (Key == EKeys::Enter || Key == EKeys::SpaceBar) {
    if (Selected != INDEX_NONE) Picked.ExecuteIfBound(Markers[Selected].Ned);
    return FReply::Handled();
  }
  return FReply::Unhandled();
}

FReply SPlaySpawnPicker::OnMouseButtonDown(const FGeometry& G, const FPointerEvent& MouseEvent) {
  const FKey Button = MouseEvent.GetEffectingButton();
  if (Button == EKeys::RightMouseButton || Button == EKeys::MiddleMouseButton) {
    bDragging = true;
    DragLast = G.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
    return FReply::Handled().CaptureMouse(AsShared());
  }
  return FReply::Handled().SetUserFocus(AsShared(), EFocusCause::Mouse);
}

FReply SPlaySpawnPicker::OnMouseButtonUp(const FGeometry& G, const FPointerEvent& MouseEvent) {
  const FKey Button = MouseEvent.GetEffectingButton();
  if (bDragging && (Button == EKeys::RightMouseButton || Button == EKeys::MiddleMouseButton)) {
    bDragging = false;
    return FReply::Handled().ReleaseMouseCapture();
  }
  if (Button == EKeys::LeftMouseButton) {
    const FVector2f Local = G.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
    const int32 Hit = MarkerAt(Local, G);
    if (Hit != INDEX_NONE) {
      Selected = Hit;
      Picked.ExecuteIfBound(Markers[Hit].Ned);
    } else {
      // Any other point: the runner re-seats the vehicle onto the ground.
      const FVector2d Ned = LocalToNed(Local, G);
      Picked.ExecuteIfBound(FVector3d(Ned.X, Ned.Y, 0.0));
    }
    return FReply::Handled();
  }
  return FReply::Unhandled();
}

FReply SPlaySpawnPicker::OnMouseMove(const FGeometry& G, const FPointerEvent& MouseEvent) {
  const FVector2f Local = G.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
  HoverLocal = Local;
  bMouseInside = true;
  if (bDragging) {
    const FVector2f Delta = Local - DragLast;
    Center.X += Delta.Y / PixelsPerMetre;
    Center.Y -= Delta.X / PixelsPerMetre;
    DragLast = Local;
    return FReply::Handled();
  }
  return FReply::Unhandled();
}

void SPlaySpawnPicker::OnMouseLeave(const FPointerEvent&) { bMouseInside = false; }

FReply SPlaySpawnPicker::OnMouseWheel(const FGeometry& G, const FPointerEvent& MouseEvent) {
  const FVector2f Local = G.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
  const FVector2d Before = LocalToNed(Local, G);
  PixelsPerMetre = FMath::Clamp(PixelsPerMetre * FMath::Pow(1.15, MouseEvent.GetWheelDelta()), 0.02, 200.0);
  const FVector2d After = LocalToNed(Local, G);
  Center += Before - After;
  return FReply::Handled();
}

// ---------------------------------------------------------------------------
// SPlayUi

void SPlayUi::Construct(const FArguments& InArgs) {
  Model = InArgs._Model;
  Actions = InArgs._Actions;
  check(Model.IsValid());
  ScreenHolder = SNew(SBox);
  HudWidget = MakeHud();
  ChildSlot[SNew(SOverlay) + SOverlay::Slot()[ScreenHolder.ToSharedRef()] + SOverlay::Slot()[HudWidget.ToSharedRef()]];
  ShowScreen(EPlayScreen::Title);
}

void SPlayUi::ShowScreen(EPlayScreen Screen) {
  Current = Screen;
  FirstFocus.Reset();
  ScreenHolder->SetContent(BuildScreen(Screen));
  bPendingFocus = FirstFocus.IsValid();
}

void SPlayUi::ApplyPendingFocus() {
  if (!bPendingFocus) return;
  if (TSharedPtr<SWidget> Target = FirstFocus.Pin()) {
    if (Target->GetPaintSpaceGeometry().GetLocalSize().X > 0.f) {
      FSlateApplication::Get().SetKeyboardFocus(Target, EFocusCause::SetDirectly);
      bPendingFocus = false;
    }
  } else {
    bPendingFocus = false;
  }
}

FReply SPlayUi::OnKeyDown(const FGeometry&, const FKeyEvent& KeyEvent) {
  if (KeyEvent.GetKey() != EKeys::Escape) return FReply::Unhandled();
  switch (Current) {
    case EPlayScreen::Map:
    case EPlayScreen::Mode:
    case EPlayScreen::Vehicle:
    case EPlayScreen::Spawn:
      if (Actions.Back) Actions.Back();
      return FReply::Handled();
    case EPlayScreen::Pause:
      if (Actions.Resume) Actions.Resume();
      return FReply::Handled();
    default:
      return FReply::Unhandled();
  }
}

TSharedRef<SWidget> SPlayUi::MakeItem(const FString& Label, bool bEnabled, TFunction<void()> Action) {
  TSharedRef<SPlayMenuItem> Item =
      SNew(SPlayMenuItem)
          .Label(FText::FromString(Label))
          .Enabled(bEnabled)
          .OnActivated_Lambda([this, Action]() {
            if (Actions.Activated) Actions.Activated();
            Action();
          })
          .OnFocused_Lambda([this]() {
            if (Actions.Focused) Actions.Focused();
          });
  if (bEnabled && !FirstFocus.IsValid()) FirstFocus = Item;
  return Item;
}

TSharedRef<SWidget> SPlayUi::MakePanel(const FString& Heading, TSharedRef<SWidget> Body) {
  return SNew(SBox).HAlign(HAlign_Left)[
      SNew(SBorder)
          .BorderImage(White())
          .BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.58f))
          .Padding(FMargin(64, 0, 96, 0))[
              SNew(SVerticalBox)
              + SVerticalBox::Slot().FillHeight(1.f)
              + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 18)[
                  SNew(STextBlock).Text(FText::FromString(Heading)).Font(Font("Regular", 13)).ColorAndOpacity(kDim)]
              + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 80)[Body]]];
}

TSharedRef<SWidget> SPlayUi::BuildScreen(EPlayScreen Screen) {
  TSharedRef<SVerticalBox> Items = SNew(SVerticalBox);
  auto Add = [&](const FString& Label, bool bEnabled, TFunction<void()> Action) {
    Items->AddSlot().AutoHeight()[MakeItem(Label, bEnabled, MoveTemp(Action))];
  };
  const FPlayUiActions A = Actions;

  switch (Screen) {
    case EPlayScreen::Title: {
      Add(TEXT("NEW FLIGHT"), true, [A]() { if (A.NewFlight) A.NewFlight(); });
      Add(TEXT("QUIT"), true, [A]() { if (A.Quit) A.Quit(); });
      return SNew(SBox).HAlign(HAlign_Left)[
          SNew(SBorder)
              .BorderImage(White())
              .BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.50f))
              .Padding(FMargin(64, 0, 120, 0))[
                  SNew(SVerticalBox)
                  + SVerticalBox::Slot().FillHeight(1.f)
                  + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 40)[Items]
                  + SVerticalBox::Slot().AutoHeight()[
                      SNew(STextBlock)
                          .Text_Lambda([M = Model]() { return FText::FromString(M->Title); })
                          .Font(Font("Light", 84))
                          .ColorAndOpacity(kInk)]
                  + SVerticalBox::Slot().AutoHeight().Padding(4, 8, 0, 56)[
                      SNew(STextBlock)
                          .Text_Lambda([M = Model]() { return FText::FromString(M->Footnote); })
                          .Font(Font("Mono", 10))
                          .ColorAndOpacity(kDim)]]];
    }
    case EPlayScreen::Map: {
      for (int32 I = 0; I < Model->MapNames.Num(); ++I) {
        Add(Model->MapNames[I], Model->MapEnabled[I], [A, I]() { if (A.PickMap) A.PickMap(I); });
      }
      Add(TEXT("BACK"), true, [A]() { if (A.Back) A.Back(); });
      return MakePanel(TEXT("MAP"), Items);
    }
    case EPlayScreen::Mode: {
      for (int32 I = 0; I < Model->ModeNames.Num(); ++I) {
        Add(Model->ModeNames[I], true, [A, I]() { if (A.PickMode) A.PickMode(I); });
      }
      Add(TEXT("BACK"), true, [A]() { if (A.Back) A.Back(); });
      return MakePanel(TEXT("MODE"), Items);
    }
    case EPlayScreen::Vehicle: {
      for (int32 I = 0; I < Model->VehicleNames.Num(); ++I) {
        Add(Model->VehicleNames[I], true, [A, I]() { if (A.PickVehicle) A.PickVehicle(I); });
      }
      Add(TEXT("BACK"), true, [A]() { if (A.Back) A.Back(); });
      return MakePanel(TEXT("VEHICLE"), Items);
    }
    case EPlayScreen::Pause: {
      Add(TEXT("RESUME"), true, [A]() { if (A.Resume) A.Resume(); });
      Add(TEXT("RESTART"), true, [A]() { if (A.Restart) A.Restart(); });
      Add(TEXT("BACK TO TITLE"), true, [A]() { if (A.BackToTitle) A.BackToTitle(); });
      return MakePanel(TEXT("PAUSED"), Items);
    }
    case EPlayScreen::Spawn: {
      TSharedRef<SPlaySpawnPicker> Picker =
          SNew(SPlaySpawnPicker)
              .Markers(Model->Markers)
              .Backdrop(Model->Backdrop)
              .BackdropNed(Model->BackdropNed)
              .OnPicked_Lambda([A](const FVector3d& Ned) { if (A.PickSpawn) A.PickSpawn(Ned); })
              .OnCancel_Lambda([A]() { if (A.Back) A.Back(); });
      FirstFocus = Picker;
      return SNew(SOverlay)
             + SOverlay::Slot()[Picker]
             + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(48, 40)[
                   SNew(SVerticalBox)
                   + SVerticalBox::Slot().AutoHeight()[
                         SNew(STextBlock).Text(FText::FromString(TEXT("SPAWN"))).Font(Font("Light", 40)).ColorAndOpacity(kInk)]
                   + SVerticalBox::Slot().AutoHeight().Padding(2, 6, 0, 0)[
                         SNew(STextBlock)
                             .Text(FText::FromString(TEXT("ARROWS OR TAB: CYCLE   ENTER OR CLICK: LAUNCH   CLICK ANYWHERE: FREE POINT   WHEEL: ZOOM   RIGHT DRAG: PAN   ESC: BACK")))
                             .Font(Font("Mono", 10))
                             .ColorAndOpacity(kDim)]];
    }
    case EPlayScreen::Loading:
      return SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)[
          SNew(SVerticalBox)
          + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[
                SNew(STextBlock).Text(FText::FromString(TEXT("LAUNCHING"))).Font(Font("Light", 48)).ColorAndOpacity(kInk)]
          + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 10)[
                SNew(STextBlock)
                    .Text_Lambda([M = Model]() { return FText::FromString(M->LoadingNote); })
                    .Font(Font("Mono", 11))
                    .ColorAndOpacity(kDim)]];
    case EPlayScreen::Flying:
      return SNullWidget::NullWidget;
  }
  return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SPlayUi::MakeHud() {
  TSharedPtr<FPlayUiModel> M = Model;
  return SNew(SBox)
      .Visibility_Lambda([this]() {
        return Current == EPlayScreen::Flying || Current == EPlayScreen::Pause || Current == EPlayScreen::Loading
                   ? EVisibility::HitTestInvisible
                   : EVisibility::Collapsed;
      })
      .HAlign(HAlign_Left)
      .VAlign(VAlign_Top)
      .Padding(FMargin(24, 18))[
          SNew(SVerticalBox)
          + SVerticalBox::Slot().AutoHeight()[
                SNew(STextBlock)
                    .Text_Lambda([M]() { return FText::FromString(M->HudStats); })
                    .Font(Font("Mono", 12))
                    .ShadowOffset(FVector2D(1, 1))
                    .ColorAndOpacity(kInk)]
          + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)[
                SNew(STextBlock)
                    .Text_Lambda([M]() { return FText::FromString(M->HudReferee); })
                    .Font(Font("Mono", 12))
                    .ShadowOffset(FVector2D(1, 1))
                    .ColorAndOpacity_Lambda([M]() {
                      if (M->HudRefereeLevel == TEXT("warn")) return FSlateColor(kWarn);
                      if (M->HudRefereeLevel == TEXT("result")) return FSlateColor(kResult);
                      return FSlateColor(kInk);
                    })]];
}
