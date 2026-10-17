// Copyright (C) 2026 Nordic Flight Stack
//
// MIT License. All rights reserved.

#include "core_sim/sensors/lens_model.hpp"

#include <algorithm>
#include <cmath>

#include "core_sim/error.hpp"

namespace microsoft {
namespace projectairsim {
namespace lens {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double ToRadians(double degrees) { return degrees * kPi / 180.0; }
// Largest source image side; beyond it the source sampling is lowered.
constexpr double kMaxSourceSide = 4096.0;
}  // namespace

LensModel ParseModel(const std::string& name) {
  if (name == "pinhole") return LensModel::kPinhole;
  if (name == "kannala-brandt") return LensModel::kKannalaBrandt;
  throw Error("Unknown lens model '" + name +
              "' (expected 'pinhole' or 'kannala-brandt').");
}

double KannalaBrandtRadius(double theta, const std::array<double, 4>& k) {
  const double t2 = theta * theta;
  return theta * (1.0 + t2 * (k[0] + t2 * (k[1] + t2 * (k[2] + t2 * k[3]))));
}

double KannalaBrandtTheta(double theta_d, const std::array<double, 4>& k) {
  double theta = theta_d;
  for (int i = 0; i < 30; ++i) {
    const double t2 = theta * theta;
    const double derivative =
        1.0 + t2 * (3 * k[0] + t2 * (5 * k[1] + t2 * (7 * k[2] + t2 * 9 * k[3])));
    if (derivative <= 0.0) {
      throw Error("Kannala-Brandt distortion is not invertible at this radius.");
    }
    const double step = (KannalaBrandtRadius(theta, k) - theta_d) / derivative;
    theta -= step;
    if (std::abs(step) < 1e-12) return theta;
  }
  throw Error("Kannala-Brandt inverse did not converge.");
}

double FocalLengthPx(const LensSettings& lens, unsigned int width,
                     double fov_degrees) {
  const double half_fov = ToRadians(fov_degrees) / 2.0;
  const double half_width = width / 2.0;
  if (lens.model == LensModel::kPinhole) return half_width / std::tan(half_fov);
  return half_width / KannalaBrandtRadius(half_fov, lens.distortion);
}

PinholeSource SourceForKannalaBrandt(const LensSettings& lens,
                                     unsigned int width, unsigned int height,
                                     double fov_degrees) {
  if (!(fov_degrees > 0.0)) {
    throw Error("A Kannala-Brandt lens needs the capture's fov-degrees.");
  }
  const double f = FocalLengthPx(lens, width, fov_degrees);
  const double max_theta = ToRadians(lens.max_incidence_degrees);
  if (max_theta <= 0.0 || max_theta >= ToRadians(89.0)) {
    throw Error("Lens max-incidence-degrees must be in (0, 89).");
  }
  // The pinhole image must reach every output pixel's ray. The corners are
  // furthest off-axis, but a ray's pinhole x and y also depend on its
  // direction, so take the extent over the whole border.
  const double cx = (width - 1) / 2.0, cy = (height - 1) / 2.0;
  double max_x = 0.0, max_y = 0.0;
  auto reach = [&](double u, double v) {
    const double x = (u - cx) / f, y = (v - cy) / f;
    const double theta_d = std::hypot(x, y);
    if (theta_d == 0.0) return;
    const double theta =
        std::min(KannalaBrandtTheta(theta_d, lens.distortion), max_theta);
    const double scale = std::tan(theta) / theta_d;
    max_x = std::max(max_x, std::abs(x) * scale);
    max_y = std::max(max_y, std::abs(y) * scale);
  };
  for (unsigned int u = 0; u < width; ++u) {
    reach(u, -0.5);
    reach(u, height - 0.5);
  }
  for (unsigned int v = 0; v < height; ++v) {
    reach(-0.5, v);
    reach(width - 0.5, v);
  }
  double focal_px = f * lens.source_sampling;
  const double side = 2.0 * std::max(max_x, max_y) * focal_px;
  if (side > kMaxSourceSide) focal_px *= kMaxSourceSide / side;

  PinholeSource source;
  // Even sizes keep the principal point on the same half-pixel grid.
  source.width = 2 * static_cast<unsigned int>(std::ceil(max_x * focal_px + 1));
  source.height = 2 * static_cast<unsigned int>(std::ceil(max_y * focal_px + 1));
  source.focal_px = focal_px;
  source.fov_degrees =
      2.0 * std::atan(source.width / 2.0 / focal_px) * 180.0 / kPi;
  return source;
}

}  // namespace lens
}  // namespace projectairsim
}  // namespace microsoft
