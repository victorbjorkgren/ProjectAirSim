// Copyright (C) 2026 Nordic Flight Stack
//
// MIT License. All rights reserved.
// Uncooled-microbolometer model: takes the radiometric render (apparent
// temperature per pixel) to what a thermal camera outputs.

#pragma once

#include <cstdint>
#include <mutex>
#include <vector>

#include "core_sim/sensors/camera.hpp"

class FThermalSensorModel {
 public:
  explicit FThermalSensorModel(
      const microsoft::projectairsim::ThermalSettings& InSettings)
      : Settings(InSettings) {}

  // Rendered apparent temperature in K, one float per pixel of Width x Height.
  // Fills Display (BGR, 3 bytes per pixel) and/or CentiKelvin (uint16 per
  // pixel) when the pointer is not null. Deterministic in (settings, frame).
  // Safe to call from several threads.
  void Process(const float* TemperatureK, int Width, int Height,
               uint64_t FrameIndex, std::vector<uint8_t>* Display,
               std::vector<uint8_t>* CentiKelvin);

  // Used by tests: the radiometric image after optics, vignette, FPN and noise.
  void Radiometric(const float* TemperatureK, int Width, int Height,
                   uint64_t FrameIndex, std::vector<float>* Out);

 private:
  void BuildFixedPattern(int Width, int Height);
  void Blur(std::vector<float>& Image, int Width, int Height, float Sigma) const;
  void ToDisplay(const std::vector<float>& Image, std::vector<uint8_t>* Out);

  microsoft::projectairsim::ThermalSettings Settings;

  std::mutex Lock;
  // Fixed-pattern noise, built on first use for the image size
  int PatternW = 0, PatternH = 0;
  std::vector<float> PixelGain, PixelOffset, ColumnOffset;
  // Display range of the previous frame, for a steady AGC
  bool bHaveRange = false;
  float RangeLow = 0.f, RangeHigh = 0.f;
  // Display gamma that puts the scene median at agc_target_median, smoothed like the range
  float RangeGamma = 1.f;
  bool bHaveGamma = false;
};
