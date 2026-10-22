// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#include "Play/UI/PlayUi.h"

#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace {
const FLinearColor kInk(0.88f, 0.89f, 0.90f, 1.f);
const FLinearColor kDim(0.88f, 0.89f, 0.90f, 0.5f);
// Menu items: the chosen one is white, the others grey.
const FLinearColor kChosen(1.f, 1.f, 1.f, 1.f);
const FLinearColor kIdle(0.80f, 0.82f, 0.84f, 0.42f);
// The Nordic brand orange of the mark, #D97706.
const FLinearColor kBrand(0.693f, 0.184f, 0.002f, 1.f);
// The dark veil over the scene behind every menu screen.
const FLinearColor kVeil(0.f, 0.f, 0.f, 0.58f);
const FLinearColor kAccent(1.00f, 0.74f, 0.25f, 1.f);
const FLinearColor kWarn(1.00f, 0.55f, 0.20f, 1.f);
const FLinearColor kResult(0.55f, 0.90f, 0.60f, 1.f);

FSlateFontInfo Font(const char* Face, int32 Size) {
  return FCoreStyle::GetDefaultFontStyle(Face, Size);
}

const FSlateBrush* White() { return FCoreStyle::Get().GetBrush("WhiteBrush"); }
}  // namespace

// ---------------------------------------------------------------------------
// SPlayLogo

namespace {
constexpr float kLogoBox = 48.f;
constexpr float kLogoCenterX = 24.f;
constexpr float kLogoStroke = 2.f;
constexpr float kLogoDash = 0.5f;  // half-length of the dash the bottom chevron starts as
constexpr float kLogoBottomBaseY = 36.f;
constexpr float kLogoBottomFlatY = kLogoBottomBaseY - 7.f;

struct FChevron {
  float HalfWidth;
  float BaseY;
  float TipY;
};

// Every chevron has a 98 degree interior angle at the tip.
FChevron MakeChevron(float HalfWidth, float BaseY) {
  const float HalfAngle = FMath::DegreesToRadians(98.f) * 0.5f;
  return {HalfWidth, BaseY, BaseY - HalfWidth / FMath::Tan(HalfAngle)};
}

float EaseInOutCubic(float T) { return T < 0.5f ? 4.f * T * T * T : 1.f - FMath::Pow(-2.f * T + 2.f, 3.f) / 2.f; }

// Left base, tip, right base, in view-box units.
TArray<FVector2f> Polyline(float HalfWidth, float LeftY, float TipY, float RightY) {
  return {FVector2f(kLogoCenterX - HalfWidth, LeftY), FVector2f(kLogoCenterX, TipY),
          FVector2f(kLogoCenterX + HalfWidth, RightY)};
}

TArray<FVector2f> Morph(const FChevron& From, const FChevron& To, float T) {
  const float Base = FMath::Lerp(From.BaseY, To.BaseY, T);
  return Polyline(FMath::Lerp(From.HalfWidth, To.HalfWidth, T), Base, FMath::Lerp(From.TipY, To.TipY, T), Base);
}
}  // namespace

void SPlayLogo::Construct(const FArguments& InArgs) {
  Progress = InArgs._Progress;
  Alpha = InArgs._Alpha;
}

int32 SPlayLogo::OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out,
                         int32 LayerId, const FWidgetStyle&, bool) const {
  const float P = FMath::Clamp(Progress.Get(), 0.f, 1.f);
  FLinearColor Color = kBrand;
  Color.A = FMath::Clamp(Alpha.Get(), 0.f, 1.f);
  if (Color.A <= 0.f) return LayerId;

  const FChevron Bottom = MakeChevron(15.f, kLogoBottomBaseY);
  const FChevron Middle = MakeChevron(12.f, kLogoBottomBaseY - 8.f);
  const FChevron Top = MakeChevron(9.f, kLogoBottomBaseY - 16.f);

  TArray<TArray<FVector2f>> Lines;
  if (P < 0.3f) {
    // A dash stretches into a flat line.
    const float T = EaseInOutCubic(P / 0.3f);
    Lines.Add(Polyline(FMath::Lerp(kLogoDash, Bottom.HalfWidth, T), kLogoBottomFlatY, kLogoBottomFlatY,
                       kLogoBottomFlatY));
  } else if (P < 0.6f) {
    // The line bends into the chevron.
    const float T = EaseInOutCubic((P - 0.3f) / 0.3f);
    Lines.Add(Polyline(Bottom.HalfWidth, FMath::Lerp(kLogoBottomFlatY, Bottom.BaseY, T),
                       FMath::Lerp(kLogoBottomFlatY, Bottom.TipY, T), FMath::Lerp(kLogoBottomFlatY, Bottom.BaseY, T)));
  } else {
    Lines.Add(Polyline(Bottom.HalfWidth, Bottom.BaseY, Bottom.TipY, Bottom.BaseY));
  }
  if (P >= 0.6f) {
    Lines.Add(P < 0.8f ? Morph(Bottom, Middle, EaseInOutCubic((P - 0.6f) / 0.2f))
                       : Polyline(Middle.HalfWidth, Middle.BaseY, Middle.TipY, Middle.BaseY));
  }
  if (P >= 0.8f) Lines.Add(Morph(Middle, Top, EaseInOutCubic((P - 0.8f) / 0.2f)));

  const FVector2f Size = G.GetLocalSize();
  const float Scale = FMath::Min(Size.X, Size.Y) / kLogoBox;
  const FVector2f Offset((Size.X - kLogoBox * Scale) * 0.5f, (Size.Y - kLogoBox * Scale) * 0.5f);
  for (TArray<FVector2f>& Line : Lines) {
    for (FVector2f& Point : Line) Point = Offset + Point * Scale;
    FSlateDrawElement::MakeLines(Out, LayerId, G.ToPaintGeometry(), Line, ESlateDrawEffect::None, Color, true,
                                 kLogoStroke * Scale);
  }
  return LayerId + 1;
}

// ---------------------------------------------------------------------------
// SPlayMenuItem

void SPlayMenuItem::Construct(const FArguments& InArgs) {
  bItemEnabled = InArgs._Enabled;
  Activated = InArgs._OnActivated;
  FocusedDelegate = InArgs._OnFocused;
  const FText Label = InArgs._Label;
  TFunction<FString()> LabelFn = InArgs._LabelFn;
  ChildSlot[SNew(SBox).Padding(FMargin(0, InArgs._FontSize < 22 ? 2 : 5))[
      SNew(STextBlock)
          .Text_Lambda([Label, LabelFn]() {
            return LabelFn ? FText::FromString(LabelFn()) : Label;
          })
          .Font(Font("Regular", InArgs._FontSize))
          .ColorAndOpacity_Lambda([this]() {
            if (!bItemEnabled) return FSlateColor(FLinearColor(1, 1, 1, 0.22f));
            return FSlateColor((HasKeyboardFocus() || IsHovered()) ? kChosen : kIdle);
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
// SPlayChannelSlider

void SPlayChannelSlider::Construct(const FArguments& InArgs) {
  Value = InArgs._Value;
  Changed = InArgs._OnChanged;
  FocusedDelegate = InArgs._OnFocused;
  const FText Label = InArgs._Label;
  const FLinearColor Tint = InArgs._Tint;
  ChildSlot[
      SNew(SHorizontalBox)
      + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[
            SNew(SBox).WidthOverride(26)[
                SNew(STextBlock)
                    .Text(Label)
                    .Font(Font("Mono", 12))
                    .ColorAndOpacity_Lambda([this]() { return FSlateColor((HasKeyboardFocus() || IsHovered()) ? kChosen : kIdle); })]]
      + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[
            SNew(SBox).HeightOverride(18)[
                SNew(SSlider)
                    .IsFocusable(false)
                    .SliderBarColor(FLinearColor(1, 1, 1, 0.18f))
                    .SliderHandleColor(Tint)
                    .Value_Lambda([this]() { return Value.Get() / 255.f; })
                    .OnValueChanged_Lambda([this](float V) { Changed.ExecuteIfBound(FMath::Clamp(FMath::RoundToInt(V * 255.f), 0, 255)); })]]
      + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10, 0, 0, 0)[
            SNew(SBox).WidthOverride(34)[
                SNew(STextBlock)
                    .Text_Lambda([this]() { return FText::AsNumber(Value.Get()); })
                    .Font(Font("Mono", 12))
                    .ColorAndOpacity(kDim)]]];
}

FReply SPlayChannelSlider::OnKeyDown(const FGeometry&, const FKeyEvent& KeyEvent) {
  const FKey Key = KeyEvent.GetKey();
  const int32 Step = KeyEvent.IsShiftDown() ? 1 : 8;
  int32 Next = Value.Get();
  if (Key == EKeys::Left) Next -= Step;
  else if (Key == EKeys::Right) Next += Step;
  else if (Key == EKeys::Home) Next = 0;
  else if (Key == EKeys::End) Next = 255;
  else return FReply::Unhandled();
  Changed.ExecuteIfBound(FMath::Clamp(Next, 0, 255));
  return FReply::Handled();
}

void SPlayChannelSlider::OnMouseEnter(const FGeometry& Geometry, const FPointerEvent& MouseEvent) {
  SCompoundWidget::OnMouseEnter(Geometry, MouseEvent);
  FSlateApplication::Get().SetKeyboardFocus(AsShared(), EFocusCause::Mouse);
}

FReply SPlayChannelSlider::OnFocusReceived(const FGeometry&, const FFocusEvent&) {
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
  ChildSlot[
      SNew(SOverlay)
      + SOverlay::Slot()[ScreenHolder.ToSharedRef()]
      + SOverlay::Slot()[MakeCurtain()]
      + SOverlay::Slot()[HudWidget.ToSharedRef()]];
  ShowScreen(EPlayScreen::Intro);
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
    case EPlayScreen::Mood:
    case EPlayScreen::Vehicle:
    case EPlayScreen::Lights:
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

TSharedRef<SWidget> SPlayUi::MakeDynamicItem(TFunction<FString()> Label, int32 FontSize, TFunction<void()> Action) {
  TSharedRef<SPlayMenuItem> Item =
      SNew(SPlayMenuItem)
          .LabelFn(Label)
          .FontSize(FontSize)
          .OnActivated_Lambda([this, Action]() {
            if (Actions.Activated) Actions.Activated();
            Action();
          })
          .OnFocused_Lambda([this]() {
            if (Actions.Focused) Actions.Focused();
          });
  if (!FirstFocus.IsValid()) FirstFocus = Item;
  return Item;
}

TSharedRef<SWidget> SPlayUi::MakePanel(const FString& Heading, TSharedRef<SWidget> Body) {
  return SNew(SBorder)
      .BorderImage(White())
      .BorderBackgroundColor(kVeil)
      .Padding(FMargin(64, 0, 96, 0))[
          SNew(SVerticalBox)
          + SVerticalBox::Slot().FillHeight(1.f)
          + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 18)[
              SNew(STextBlock).Text(FText::FromString(Heading)).Font(Font("Regular", 13)).ColorAndOpacity(kDim)]
          + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 80)[Body]];
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
      return SNew(SBorder)
          .BorderImage(White())
          .BorderBackgroundColor(kVeil)
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
                      .ColorAndOpacity(kDim)]];
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
    case EPlayScreen::Mood: {
      for (int32 I = 0; I < Model->MoodNames.Num(); ++I) {
        Add(Model->MoodNames[I], true, [A, I]() { if (A.PickMood) A.PickMood(I); });
        // The current mood is where the keyboard starts.
        if (I == Model->CurrentMood) FirstFocus = Items->GetChildren()->GetChildAt(Items->GetChildren()->Num() - 1);
      }
      Add(TEXT("BACK"), true, [A]() { if (A.Back) A.Back(); });
      return MakePanel(TEXT("MOOD"), Items);
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
      if (Model->MoodNames.Num() > 0) {
        const FString MoodName = Model->MoodNames.IsValidIndex(Model->CurrentMood) ? Model->MoodNames[Model->CurrentMood] : FString(TEXT("DEFAULT"));
        Add(FString::Printf(TEXT("MOOD: %s"), *MoodName), true, [A]() { if (A.OpenMood) A.OpenMood(); });
      }
      Add(TEXT("LIGHTS"), true, [A]() { if (A.OpenLights) A.OpenLights(); });
      Add(TEXT("BACK TO TITLE"), true, [A]() { if (A.BackToTitle) A.BackToTitle(); });
      return MakePanel(TEXT("PAUSED"), Items);
    }
    case EPlayScreen::Lights:
      return BuildLights();
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
    case EPlayScreen::Intro:
    case EPlayScreen::Flying:
      return SNullWidget::NullWidget;
  }
  return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SPlayUi::MakeCurtain() {
  TSharedPtr<FPlayUiModel> M = Model;
  return SNew(SOverlay)
      .Visibility_Lambda([M]() {
        return M->CurtainAlpha > 0.f ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
      })
      + SOverlay::Slot()[
            SNew(SBorder)
                .BorderImage(White())
                .BorderBackgroundColor_Lambda([M]() { return FLinearColor(0.f, 0.f, 0.f, M->CurtainAlpha); })]
      + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[
            SNew(SBox).WidthOverride(140).HeightOverride(140)[
                SNew(SPlayLogo)
                    .Progress_Lambda([M]() { return M->LogoProgress; })
                    .Alpha_Lambda([M]() { return M->LogoAlpha; })]];
}

TSharedRef<SWidget> SPlayUi::MakeHud() {
  TSharedPtr<FPlayUiModel> M = Model;
  return SNew(SBox)
      .Visibility_Lambda([this]() {
        return Current == EPlayScreen::Flying || Current == EPlayScreen::Pause || Current == EPlayScreen::Lights ||
                       Current == EPlayScreen::Loading
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

TSharedRef<SWidget> SPlayUi::BuildLights() {
  check(Model->Lights.IsValid() && Model->LightSettings);
  const TSharedPtr<FPlayLightsState> L = Model->Lights;
  const UPlaySettings* Settings = Model->LightSettings;
  const FPlayUiActions A = Actions;
  constexpr int32 kRowFont = 15;

  TSharedRef<SVerticalBox> Col = SNew(SVerticalBox);
  auto Row = [&](TFunction<FString()> Label, TFunction<void()> Action) {
    Col->AddSlot().AutoHeight()[MakeDynamicItem(MoveTemp(Label), kRowFont, MoveTemp(Action))];
  };
  Row([L]() { return FString::Printf(TEXT("FLASHLIGHT (F): %s"), !L->bFlashlight ? TEXT("OFF") : L->bFlashlightYielded ? TEXT("ON - PROJECT LAMP SHOWN") : TEXT("ON")); },
      [A]() { if (A.LightsFlashlight) A.LightsFlashlight(); });
  Row([L]() { return FString::Printf(TEXT("NAV LIGHTS (N): %s"), L->bNav ? TEXT("ON") : TEXT("OFF")); },
      [A]() { if (A.LightsNav) A.LightsNav(); });
  Row([L]() { return FString::Printf(TEXT("PATTERN (V): %s"), FPlayLights::ModeName(L->Mode)); },
      [A]() { if (A.LightsMode) A.LightsMode(); });
  Row([L, Settings]() { return FString::Printf(TEXT("PRESET (B): %s"), *L->PresetName(*Settings)); },
      [A]() { if (A.LightsPreset) A.LightsPreset(); });

  // Live preview: the three lights as seen from above, blinking as the pattern says.
  auto Light = [L](int32 Role) {
    return [L, Role]() {
      const FColor& C = Role == 0 ? L->Left : Role == 1 ? L->Right : L->Tail;
      const bool bPresent = Role != 2 || L->bTail;
      const float F = bPresent ? FMath::Min(FPlayLights::Pattern(L->Mode, Role, L->bTail, FPlatformTime::Seconds()), 1.f) : 0.f;
      FLinearColor Colour = FLinearColor::FromSRGBColor(C) * F;
      Colour.A = 1.f;
      return Colour;
    };
  };
  auto Dot = [&](int32 Role, float Size) -> TSharedRef<SWidget> {
    return SNew(SBox).WidthOverride(Size).HeightOverride(Size)[
        SNew(SBorder).BorderImage(White()).BorderBackgroundColor_Lambda(Light(Role))];
  };
  Col->AddSlot().AutoHeight().Padding(0, 14, 0, 8)[
      SNew(SHorizontalBox)
      + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[
            SNew(STextBlock).Text(FText::FromString(TEXT("PREVIEW"))).Font(Font("Mono", 11)).ColorAndOpacity(kDim)]
      + SHorizontalBox::Slot().AutoWidth().Padding(18, 0, 0, 0)[Dot(0, 34)]
      + SHorizontalBox::Slot().AutoWidth().Padding(26, 0, 0, 0).VAlign(VAlign_Center)[Dot(2, 22)]
      + SHorizontalBox::Slot().AutoWidth().Padding(26, 0, 0, 0)[Dot(1, 34)]
      + SHorizontalBox::Slot().AutoWidth().Padding(14, 0, 0, 0).VAlign(VAlign_Center)[
            SNew(STextBlock).Text(FText::FromString(TEXT("LEFT  TAIL  RIGHT"))).Font(Font("Mono", 10)).ColorAndOpacity(kDim)]];

  auto Get = [](const FColor& C, int32 I) { return static_cast<int32>(I == 0 ? C.R : I == 1 ? C.G : C.B); };
  auto Block = [&](const TCHAR* Name, FColor* Colour) {
    const TCHAR* Letters[3] = {TEXT("R"), TEXT("G"), TEXT("B")};
    const FLinearColor Tints[3] = {FLinearColor(1.f, 0.35f, 0.3f), FLinearColor(0.4f, 1.f, 0.45f), FLinearColor(0.4f, 0.6f, 1.f)};
    TSharedRef<SVerticalBox> Channels = SNew(SVerticalBox);
    for (int32 I = 0; I < 3; ++I) {
      TSharedRef<SPlayChannelSlider> Slider =
          SNew(SPlayChannelSlider)
              .Label(FText::FromString(Letters[I]))
              .Tint(Tints[I])
              .Value_Lambda([Colour, I, Get]() { return Get(*Colour, I); })
              .OnChanged_Lambda([this, Colour, I, A](int32 V) {
                (I == 0 ? Colour->R : I == 1 ? Colour->G : Colour->B) = static_cast<uint8>(V);
                if (A.LightsEdited) A.LightsEdited();
              })
              .OnFocused_Lambda([this]() { if (Actions.Focused) Actions.Focused(); });
      Channels->AddSlot().AutoHeight().Padding(0, 1)[Slider];
    }
    Col->AddSlot().AutoHeight().Padding(0, 10, 0, 0)[
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 14, 0)[
              SNew(SBox).WidthOverride(86)[
                  SNew(SVerticalBox)
                  + SVerticalBox::Slot().AutoHeight()[
                        SNew(STextBlock).Text(FText::FromString(Name)).Font(Font("Regular", 15)).ColorAndOpacity(kInk)]
                  + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)[
                        SNew(SBox).HeightOverride(22)[
                            SNew(SBorder).BorderImage(White()).BorderBackgroundColor_Lambda(
                                [Colour]() { FLinearColor C = FLinearColor::FromSRGBColor(*Colour); C.A = 1.f; return C; })]]]]
        + SHorizontalBox::Slot().FillWidth(1.f)[Channels]];
  };
  Block(TEXT("LEFT"), &L->Left);
  Block(TEXT("RIGHT"), &L->Right);
  Block(TEXT("TAIL"), &L->Tail);
  Col->AddSlot().AutoHeight().Padding(0, 8, 0, 0)[MakeDynamicItem(
      [L]() { return FString::Printf(TEXT("TAIL LIGHT: %s"), L->bTail ? TEXT("ON") : TEXT("OFF")); }, kRowFont,
      [A]() { if (A.LightsTail) A.LightsTail(); })];
  Col->AddSlot().AutoHeight()[MakeDynamicItem([]() { return FString(TEXT("BACK")); }, kRowFont, [A]() { if (A.Back) A.Back(); })];
  Col->AddSlot().AutoHeight().Padding(0, 10, 0, 0)[
      SNew(STextBlock)
          .Text(FText::FromString(TEXT("UP/DOWN: SELECT   LEFT/RIGHT: ADJUST (SHIFT: FINE)   ENTER: TOGGLE   ESC: BACK")))
          .Font(Font("Mono", 9))
          .ColorAndOpacity(kDim)];

  return SNew(SBox).HAlign(HAlign_Left)[
      SNew(SBox).WidthOverride(560)[
          SNew(SBorder)
              .BorderImage(White())
              .BorderBackgroundColor(kVeil)
              .Padding(FMargin(40, 80, 40, 24))[
                  SNew(SVerticalBox)
                  + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)[
                        SNew(STextBlock).Text(FText::FromString(TEXT("LIGHTS"))).Font(Font("Light", 40)).ColorAndOpacity(kInk)]
                  + SVerticalBox::Slot().FillHeight(1.f)[Col]]]];
}
