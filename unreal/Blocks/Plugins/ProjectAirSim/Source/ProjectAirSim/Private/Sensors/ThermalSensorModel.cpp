// Copyright (C) 2026 Nordic Flight Stack
//
// MIT License. All rights reserved.

#include "ThermalSensorModel.h"

#include <algorithm>
#include <cmath>

namespace {

using microsoft::projectairsim::ThermalSettings;

// Counter-based generator: the same (seed, stream, index) always gives the same value.
uint32_t Hash(uint32_t Seed, uint32_t Stream, uint32_t Index) {
  uint32_t X = Seed * 0x9E3779B1u ^ Stream * 0x85EBCA6Bu ^ Index * 0xC2B2AE35u;
  X ^= X >> 16; X *= 0x7FEB352Du; X ^= X >> 15; X *= 0x846CA68Bu; X ^= X >> 16;
  return X;
}

// Standard normal from two hashes (Box-Muller).
float Gaussian(uint32_t Seed, uint32_t Stream, uint32_t Index) {
  const float U1 = (Hash(Seed, Stream * 2 + 1, Index) + 1.0f) / 4294967296.0f;
  const float U2 = Hash(Seed, Stream * 2 + 2, Index) / 4294967296.0f;
  return std::sqrt(-2.0f * std::log(U1)) * std::cos(6.2831853f * U2);
}

struct Rgb { float R, G, B; };

// Ironbow-like palette: black, indigo, magenta, orange, yellow, white.
Rgb Ironbow(float V) {
  static const float Stops[6][3] = {{0, 0, 0}, {0.18f, 0.02f, 0.45f}, {0.65f, 0.1f, 0.5f},
                                    {0.95f, 0.4f, 0.1f}, {1.0f, 0.85f, 0.2f}, {1, 1, 1}};
  const float P = std::clamp(V, 0.0f, 1.0f) * 5.0f;
  const int I = std::min(4, static_cast<int>(P));
  const float F = P - I;
  return {Stops[I][0] + (Stops[I + 1][0] - Stops[I][0]) * F,
          Stops[I][1] + (Stops[I + 1][1] - Stops[I][1]) * F,
          Stops[I][2] + (Stops[I + 1][2] - Stops[I][2]) * F};
}

}  // namespace

void FThermalSensorModel::BuildFixedPattern(int Width, int Height) {
  if (PatternW == Width && PatternH == Height) return;
  PatternW = Width;
  PatternH = Height;
  PixelGain.resize(size_t(Width) * Height);
  PixelOffset.resize(size_t(Width) * Height);
  ColumnOffset.resize(Width);
  for (size_t I = 0; I < PixelGain.size(); ++I) {
    PixelGain[I] = 1.0f + Settings.fpn_gain_sigma * Gaussian(Settings.seed, 1, uint32_t(I));
    PixelOffset[I] = Settings.fpn_offset_kelvin * Gaussian(Settings.seed, 2, uint32_t(I));
  }
  for (int X = 0; X < Width; ++X) {
    ColumnOffset[X] = Settings.fpn_column_kelvin * Gaussian(Settings.seed, 3, uint32_t(X));
  }
}

void FThermalSensorModel::Blur(std::vector<float>& Image, int W, int H, float Sigma) const {
  if (Sigma < 0.05f) return;
  const int Radius = std::max(1, int(std::ceil(3.0f * Sigma)));
  std::vector<float> Kernel(2 * Radius + 1);
  float Sum = 0;
  for (int I = -Radius; I <= Radius; ++I) {
    Kernel[I + Radius] = std::exp(-0.5f * I * I / (Sigma * Sigma));
    Sum += Kernel[I + Radius];
  }
  for (float& K : Kernel) K /= Sum;
  std::vector<float> Tmp(Image.size());
  for (int Y = 0; Y < H; ++Y) {
    for (int X = 0; X < W; ++X) {
      float A = 0;
      for (int I = -Radius; I <= Radius; ++I) {
        A += Kernel[I + Radius] * Image[size_t(Y) * W + std::clamp(X + I, 0, W - 1)];
      }
      Tmp[size_t(Y) * W + X] = A;
    }
  }
  for (int Y = 0; Y < H; ++Y) {
    for (int X = 0; X < W; ++X) {
      float A = 0;
      for (int I = -Radius; I <= Radius; ++I) {
        A += Kernel[I + Radius] * Tmp[size_t(std::clamp(Y + I, 0, H - 1)) * W + X];
      }
      Image[size_t(Y) * W + X] = A;
    }
  }
}

void FThermalSensorModel::Radiometric(const float* Source, int W, int H, uint64_t Frame,
                                      std::vector<float>* Out) {
  std::vector<float>& T = *Out;
  T.assign(Source, Source + size_t(W) * H);

  double Mean = 0;
  for (float V : T) Mean += V;
  const float Tm = float(Mean / T.size());

  // Optics: blur, then vignetting (the signal around the scene mean falls toward the corners).
  Blur(T, W, H, Settings.psf_sigma_px);
  if (Settings.vignette > 0) {
    const float Cx = (W - 1) * 0.5f, Cy = (H - 1) * 0.5f, R2 = Cx * Cx + Cy * Cy;
    for (int Y = 0; Y < H; ++Y) {
      for (int X = 0; X < W; ++X) {
        const float R = ((X - Cx) * (X - Cx) + (Y - Cy) * (Y - Cy)) / R2;
        float& V = T[size_t(Y) * W + X];
        V = Tm + (V - Tm) * (1.0f - Settings.vignette * R);
      }
    }
  }
  // Image processing of the camera: edge sharpening that leaves a halo.
  if (Settings.halo > 0) {
    std::vector<float> Soft = T;
    Blur(Soft, W, H, 2.0f);
    for (size_t I = 0; I < T.size(); ++I) T[I] += Settings.halo * (T[I] - Soft[I]);
  }
  if (Settings.noise_enabled) {
    {
      std::lock_guard<std::mutex> Guard(Lock);
      BuildFixedPattern(W, H);
    }
    for (int Y = 0; Y < H; ++Y) {
      for (int X = 0; X < W; ++X) {
        const size_t I = size_t(Y) * W + X;
        float V = Tm + (T[I] - Tm) * PixelGain[I] + PixelOffset[I] + ColumnOffset[X];
        // Temporal noise: its own stream per frame.
        V += Settings.netd_kelvin * Gaussian(Settings.seed, 4 + uint32_t(Frame & 0xFFFFFF) * 2 + 1000, uint32_t(I));
        T[I] = V;
      }
    }
  }
}

void FThermalSensorModel::ToDisplay(const std::vector<float>& T, std::vector<uint8_t>* Out) {
  const size_t N = T.size();
  constexpr float kBase = 150.0f, kBin = 0.02f;
  constexpr int kBins = 15000;  // 150..450 K in 0.02 K
  std::vector<uint32_t> Hist(kBins, 0);
  for (float V : T) {
    Hist[std::clamp(int((V - kBase) / kBin), 0, kBins - 1)]++;
  }
  auto Percentile = [&](float P) {
    const uint64_t Target = std::max<uint64_t>(1, uint64_t(P * 0.01 * N));
    uint64_t Acc = 0;
    for (int B = 0; B < kBins; ++B) {
      Acc += Hist[B];
      if (Acc >= Target) return kBase + (B + 0.5f) * kBin;
    }
    return kBase + kBins * kBin;
  };

  float Lo, Hi;
  if (Settings.agc == ThermalSettings::Agc::kFixed) {
    Lo = Settings.fixed_min_celsius + 273.15f;
    Hi = Settings.fixed_max_celsius + 273.15f;
  } else {
    Lo = Percentile(Settings.agc_low_percent);
    Hi = Percentile(Settings.agc_high_percent);
    if (Hi - Lo < Settings.agc_min_span_kelvin) {
      const float Mid = 0.5f * (Lo + Hi);
      Lo = Mid - 0.5f * Settings.agc_min_span_kelvin;
      Hi = Mid + 0.5f * Settings.agc_min_span_kelvin;
    }
    std::lock_guard<std::mutex> Guard(Lock);
    if (bHaveRange) {
      const float S = std::clamp(Settings.agc_smoothing, 0.0f, 0.999f);
      Lo = S * RangeLow + (1 - S) * Lo;
      Hi = S * RangeHigh + (1 - S) * Hi;
    }
    RangeLow = Lo;
    RangeHigh = Hi;
    bHaveRange = true;
  }

  // Histogram equalisation maps through a clipped cumulative distribution (plateau equalisation:
  // the clip keeps large uniform areas from swallowing the contrast and noise from being amplified).
  std::vector<float> Lut;
  if (Settings.agc == ThermalSettings::Agc::kHistogramEqualization) {
    constexpr int kLutBins = 512;
    std::vector<float> H(kLutBins, 0.f);
    for (float V : T) {
      const float U = (V - Lo) / (Hi - Lo);
      H[std::clamp(int(U * kLutBins), 0, kLutBins - 1)] += 1.f;
    }
    const float Plateau = 2.0f * float(N) / kLutBins;
    float Sum = 0;
    for (float& V : H) { V = std::min(V, Plateau); Sum += V; }
    Lut.resize(kLutBins);
    float Acc = 0;
    for (int B = 0; B < kLutBins; ++B) { Acc += H[B]; Lut[B] = Acc / Sum; }
  }

  Out->resize(N * 3);
  for (size_t I = 0; I < N; ++I) {
    float U = std::clamp((T[I] - Lo) / (Hi - Lo), 0.0f, 1.0f);
    // Half equalised, half linear: full equalisation posterises smooth gradients.
    if (!Lut.empty()) U = 0.5f * U + 0.5f * Lut[std::min(int(U * Lut.size()), int(Lut.size()) - 1)];
    // 8-bit display: the camera outputs a grey level.
    const float G = std::round(U * 255.0f) / 255.0f;
    Rgb C;
    switch (Settings.palette) {
      case ThermalSettings::Palette::kBlackHot: C = {1 - G, 1 - G, 1 - G}; break;
      case ThermalSettings::Palette::kIronbow: C = Ironbow(G); break;
      default: C = {G, G, G};
    }
    uint8_t* P = Out->data() + I * 3;
    P[0] = uint8_t(std::round(C.B * 255.0f));
    P[1] = uint8_t(std::round(C.G * 255.0f));
    P[2] = uint8_t(std::round(C.R * 255.0f));
  }
}

void FThermalSensorModel::Process(const float* Source, int W, int H, uint64_t Frame,
                                  std::vector<uint8_t>* Display,
                                  std::vector<uint8_t>* CentiKelvin) {
  std::vector<float> T;
  Radiometric(Source, W, H, Frame, &T);
  if (CentiKelvin) {
    CentiKelvin->resize(T.size() * 2);
    uint16_t* Dst = reinterpret_cast<uint16_t*>(CentiKelvin->data());
    for (size_t I = 0; I < T.size(); ++I) {
      Dst[I] = uint16_t(std::clamp(std::lround(T[I] * 100.0f), 0L, 65535L));
    }
  }
  if (Display) ToDisplay(T, Display);
}
