// Copyright (C) 2026 Nordic Flight Stack
//
// MIT License. All rights reserved.

#include "LensRemap.h"

#include <cmath>

#include "Async/ParallelFor.h"
#include "core_sim/sensors/lens_model.hpp"

namespace projectairsim = microsoft::projectairsim;
namespace lens = microsoft::projectairsim::lens;

TSharedPtr<const FLensRemap> FLensRemap::Create(
    const projectairsim::CaptureSettings& Setting) {
  if (!lens::NeedsRemap(Setting.lens)) return nullptr;

  const auto Source = lens::RenderSource(
      Setting.lens, Setting.width, Setting.height, Setting.fov_degrees);
  TSharedPtr<FLensRemap> Map = MakeShared<FLensRemap>();
  Map->Width = Setting.width;
  Map->Height = Setting.height;
  Map->SourceWidth = Source.width;
  Map->SourceHeight = Source.height;
  Map->SourceFovDegrees = Source.fov_degrees;

  const double F = lens::FocalLengthPx(Setting.lens, Setting.width,
                                       Setting.fov_degrees);
  const double Cx = (Map->Width - 1) / 2.0, Cy = (Map->Height - 1) / 2.0;
  const double SourceCx = (Map->SourceWidth - 1) / 2.0;
  const double SourceCy = (Map->SourceHeight - 1) / 2.0;
  const double MaxTheta = Setting.lens.max_incidence_degrees * PI / 180.0;
  const auto& K = Setting.lens.distortion;
  const bool bPinhole = Setting.lens.model == projectairsim::LensModel::kPinhole;

  // Output pixel position -> source pinhole position along the same ray.
  auto ToSource = [&](double U, double V) {
    const double X = (U - Cx) / F, Y = (V - Cy) / F;
    // A supersampled pinhole: the same ray in the larger pinhole image.
    if (bPinhole) {
      return FVector2f(SourceCx + X * Source.focal_px,
                       SourceCy + Y * Source.focal_px);
    }
    const double ThetaD = std::hypot(X, Y);
    if (ThetaD < 1e-12) return FVector2f(SourceCx, SourceCy);
    const double Theta = lens::KannalaBrandtTheta(ThetaD, K);
    if (Theta > MaxTheta) return FVector2f(-1.0f, -1.0f);
    const double Scale = std::tan(Theta) / ThetaD * Source.focal_px;
    return FVector2f(SourceCx + X * Scale, SourceCy + Y * Scale);
  };

  static constexpr double kOffsets[kTaps][2] = {
      {-0.25, -0.25}, {0.25, -0.25}, {-0.25, 0.25}, {0.25, 0.25}};
  Map->Taps.SetNumUninitialized(Map->Width * Map->Height * kTaps);
  Map->Nearest.SetNumUninitialized(Map->Width * Map->Height);
  for (int32 V = 0; V < Map->Height; ++V) {
    for (int32 U = 0; U < Map->Width; ++U) {
      const int32 Pixel = V * Map->Width + U;
      for (int32 T = 0; T < kTaps; ++T) {
        Map->Taps[Pixel * kTaps + T] =
            ToSource(U + kOffsets[T][0], V + kOffsets[T][1]);
      }
      const FVector2f Center = ToSource(U, V);
      const int32 Sx = FMath::RoundToInt(Center.X);
      const int32 Sy = FMath::RoundToInt(Center.Y);
      Map->Nearest[Pixel] = (Center.X < 0.0f || Sx >= Map->SourceWidth ||
                             Sy < 0 || Sy >= Map->SourceHeight)
                                ? INDEX_NONE
                                : Sy * Map->SourceWidth + Sx;
    }
  }
  return Map;
}

FIntPoint FLensRemap::RenderSize(const projectairsim::CaptureSettings& Setting) {
  if (!lens::NeedsRemap(Setting.lens)) {
    return FIntPoint(Setting.width, Setting.height);
  }
  const auto Source = lens::RenderSource(
      Setting.lens, Setting.width, Setting.height, Setting.fov_degrees);
  return FIntPoint(Source.width, Source.height);
}

float FLensRemap::RenderFovDegrees(
    const projectairsim::CaptureSettings& Setting) {
  if (!lens::NeedsRemap(Setting.lens)) return Setting.fov_degrees;
  return lens::RenderSource(Setting.lens, Setting.width, Setting.height,
                            Setting.fov_degrees)
      .fov_degrees;
}

void FLensRemap::Apply(const TArray<FColor>& Source,
                       TArray<FColor>& Output) const {
  check(Source.Num() == SourceWidth * SourceHeight);
  Output.SetNumUninitialized(Width * Height);
  const int32 MaxX = SourceWidth - 1, MaxY = SourceHeight - 1;
  ParallelFor(Height, [&](int32 V) {
    for (int32 U = 0; U < Width; ++U) {
      const int32 Pixel = V * Width + U;
      float Sum[4] = {0, 0, 0, 0};
      int32 Inside = 0;
      for (int32 T = 0; T < kTaps; ++T) {
        const FVector2f P = Taps[Pixel * kTaps + T];
        if (P.X < 0.0f) continue;
        const int32 X0 = FMath::Clamp(FMath::FloorToInt(P.X), 0, MaxX);
        const int32 Y0 = FMath::Clamp(FMath::FloorToInt(P.Y), 0, MaxY);
        const int32 X1 = FMath::Min(X0 + 1, MaxX), Y1 = FMath::Min(Y0 + 1, MaxY);
        const float Ax = FMath::Clamp(P.X - X0, 0.0f, 1.0f);
        const float Ay = FMath::Clamp(P.Y - Y0, 0.0f, 1.0f);
        const FColor& C00 = Source[Y0 * SourceWidth + X0];
        const FColor& C10 = Source[Y0 * SourceWidth + X1];
        const FColor& C01 = Source[Y1 * SourceWidth + X0];
        const FColor& C11 = Source[Y1 * SourceWidth + X1];
        const float W00 = (1 - Ax) * (1 - Ay), W10 = Ax * (1 - Ay);
        const float W01 = (1 - Ax) * Ay, W11 = Ax * Ay;
        Sum[0] += W00 * C00.B + W10 * C10.B + W01 * C01.B + W11 * C11.B;
        Sum[1] += W00 * C00.G + W10 * C10.G + W01 * C01.G + W11 * C11.G;
        Sum[2] += W00 * C00.R + W10 * C10.R + W01 * C01.R + W11 * C11.R;
        Sum[3] += W00 * C00.A + W10 * C10.A + W01 * C01.A + W11 * C11.A;
        ++Inside;
      }
      // Partly covered edge pixels of the image circle fade to black.
      const float Norm = 1.0f / kTaps;
      Output[Pixel] = FColor(FMath::RoundToInt(Sum[2] * Norm),
                             FMath::RoundToInt(Sum[1] * Norm),
                             FMath::RoundToInt(Sum[0] * Norm),
                             Inside ? FMath::RoundToInt(Sum[3] / Inside) : 255);
    }
  });
}

void FLensRemap::ApplyNearest(const TArray<FColor>& Source,
                              TArray<FColor>& Output) const {
  check(Source.Num() == SourceWidth * SourceHeight);
  Output.SetNumUninitialized(Width * Height);
  ParallelFor(Height, [&](int32 V) {
    for (int32 Pixel = V * Width; Pixel < (V + 1) * Width; ++Pixel) {
      Output[Pixel] =
          Nearest[Pixel] == INDEX_NONE ? FColor::Black : Source[Nearest[Pixel]];
    }
  });
}

void FLensRemap::ApplyNearest(const TArray<FFloat16Color>& Source,
                              TArray<FFloat16Color>& Output) const {
  check(Source.Num() == SourceWidth * SourceHeight);
  Output.SetNumUninitialized(Width * Height);
  ParallelFor(Height, [&](int32 V) {
    for (int32 Pixel = V * Width; Pixel < (V + 1) * Width; ++Pixel) {
      Output[Pixel] = Nearest[Pixel] == INDEX_NONE ? FFloat16Color()
                                                   : Source[Nearest[Pixel]];
    }
  });
}
