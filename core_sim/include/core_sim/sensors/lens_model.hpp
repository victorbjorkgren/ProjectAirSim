// Copyright (C) 2026 Nordic Flight Stack
//
// MIT License. All rights reserved.
// Camera lens models: pinhole and Kannala-Brandt (OpenCV cv::fisheye, ROS
// "equidistant").

#ifndef CORE_SIM_INCLUDE_CORE_SIM_SENSORS_LENS_MODEL_HPP_
#define CORE_SIM_INCLUDE_CORE_SIM_SENSORS_LENS_MODEL_HPP_

#include <array>
#include <string>

namespace microsoft {
namespace projectairsim {

enum class LensModel { kPinhole = 0, kKannalaBrandt = 1 };

// How a capture's image maps rays to pixels.
//
// Pinhole: the rendered image as is, r = f tan(theta).
// Kannala-Brandt: r = f theta_d with
//   theta_d = theta (1 + k1 theta^2 + k2 theta^4 + k3 theta^6 + k4 theta^8),
// theta the angle of the ray from the optical axis. k = 0 is an ideal
// equidistant fisheye. The focal length f follows from the capture's
// fov-degrees: the left and right image edges are fov/2 off-axis. The
// principal point is the image centre, cx = (width - 1) / 2 and
// cy = (height - 1) / 2 in pixel-centre coordinates.
struct LensSettings {
  LensModel model = LensModel::kPinhole;
  std::array<double, 4> distortion = {0.0, 0.0, 0.0, 0.0};  // k1..k4
  // Rays further off-axis than this render black, as outside a lens's image
  // circle. It also bounds the pinhole image rendered for the remap.
  double max_incidence_degrees = 80.0;
  // Rendered pinhole pixels per output pixel at the image centre. Below 1 the
  // centre is upsampled (softer); above 1 costs render time. For a pinhole it
  // is 1 or 2; 2 supersamples: the image is rendered at twice the width and
  // height and each output pixel is the mean of its 2x2 source pixels.
  double source_sampling = 1.0;
};

namespace lens {

// "pinhole" or "kannala-brandt"; throws on anything else.
LensModel ParseModel(const std::string& name);

// theta_d of the Kannala-Brandt polynomial for a ray at angle theta (radians).
double KannalaBrandtRadius(double theta, const std::array<double, 4>& k);

// The ray angle theta whose theta_d is the given value (Newton iteration).
// Throws if the polynomial is not invertible there.
double KannalaBrandtTheta(double theta_d, const std::array<double, 4>& k);

// Focal length in pixels for an image of the given width whose left and right
// edges are fov_degrees/2 off-axis.
double FocalLengthPx(const LensSettings& lens, unsigned int width,
                     double fov_degrees);

// Size and horizontal field of view of the pinhole image to render so that a
// Kannala-Brandt output image of the given size can be remapped from it.
struct PinholeSource {
  unsigned int width = 0;
  unsigned int height = 0;
  double fov_degrees = 0.0;
  double focal_px = 0.0;
};
PinholeSource SourceForKannalaBrandt(const LensSettings& lens,
                                     unsigned int width, unsigned int height,
                                     double fov_degrees);

// Whether the capture renders a different pinhole image than its output and
// remaps it: a Kannala-Brandt lens, or a supersampled pinhole.
bool NeedsRemap(const LensSettings& lens);

// The pinhole image to render for a capture that needs a remap, for either
// model. Throws for a source_sampling that is not > 0, or not 1 or 2 for a
// pinhole.
PinholeSource RenderSource(const LensSettings& lens, unsigned int width,
                           unsigned int height, double fov_degrees);

}  // namespace lens
}  // namespace projectairsim
}  // namespace microsoft

#endif  // CORE_SIM_INCLUDE_CORE_SIM_SENSORS_LENS_MODEL_HPP_
