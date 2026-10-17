// Copyright (C) 2026 Nordic Flight Stack
//
// MIT License. All rights reserved.
// Remaps a rendered pinhole image to a capture's lens model.

#pragma once

#include "CoreMinimal.h"
#include "core_sim/sensors/camera.hpp"

// Unreal renders pinhole images only. For a Kannala-Brandt (fisheye) capture,
// the camera renders a wider pinhole image (SourceWidth x SourceHeight at
// SourceFovDegrees) and every output pixel is looked up in it along its ray.
// A supersampled pinhole capture renders a larger image of the same view and
// averages it down the same way.
// The map is built once per capture; Apply runs per frame on the packing
// thread.
class FLensRemap {
 public:
  // nullptr for a plain pinhole capture: the rendered image is the output.
  static TSharedPtr<const FLensRemap> Create(
      const microsoft::projectairsim::CaptureSettings& Setting);

  // Size and horizontal field of view of the image Unreal renders for the
  // capture: the capture's own for a plain pinhole, else the remap source.
  static FIntPoint RenderSize(
      const microsoft::projectairsim::CaptureSettings& Setting);
  static float RenderFovDegrees(
      const microsoft::projectairsim::CaptureSettings& Setting);

  // Colour images: the mean of four bilinear samples per output pixel, so
  // the minified edges of the source do not alias. Rays outside the lens's
  // max incidence are black.
  void Apply(const TArray<FColor>& Source, TArray<FColor>& Output) const;
  // Depth, segmentation and other images whose values must not be blended:
  // the nearest source pixel.
  void ApplyNearest(const TArray<FColor>& Source, TArray<FColor>& Output) const;
  void ApplyNearest(const TArray<FFloat16Color>& Source,
                    TArray<FFloat16Color>& Output) const;

  int32 Width = 0, Height = 0;              // output (lens) image
  int32 SourceWidth = 0, SourceHeight = 0;  // rendered pinhole image
  float SourceFovDegrees = 0.0f;

 private:
  static constexpr int32 kTaps = 4;
  // kTaps source positions per output pixel, in source pixel-centre
  // coordinates; X < 0 marks a ray outside the lens.
  TArray<FVector2f> Taps;
  // Nearest source pixel index per output pixel; INDEX_NONE outside the lens.
  TArray<int32> Nearest;
};
