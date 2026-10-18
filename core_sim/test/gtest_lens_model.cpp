// Copyright (C) 2026 Nordic Flight Stack
//
// MIT License. All rights reserved.
// Tests for the camera lens models

#include <cmath>

#include "core_sim/error.hpp"
#include "core_sim/sensors/lens_model.hpp"
#include "gtest/gtest.h"

namespace projectairsim = microsoft::projectairsim;
namespace lens = microsoft::projectairsim::lens;

namespace {
constexpr double kPi = 3.14159265358979323846;
// Coefficients of the size a real fisheye calibration gives (cv::fisheye).
const std::array<double, 4> kLensK = {-0.013, 0.021, -0.017, 0.0049};
}  // namespace

TEST(LensModel, ParsesModelNames) {
  EXPECT_EQ(lens::ParseModel("pinhole"), projectairsim::LensModel::kPinhole);
  EXPECT_EQ(lens::ParseModel("kannala-brandt"),
            projectairsim::LensModel::kKannalaBrandt);
  EXPECT_THROW(lens::ParseModel("fisheye"), projectairsim::Error);
}

TEST(LensModel, KannalaBrandtInverseRoundTrips) {
  for (double theta = 0.0; theta < 1.5; theta += 0.05) {
    const double theta_d = lens::KannalaBrandtRadius(theta, kLensK);
    EXPECT_NEAR(lens::KannalaBrandtTheta(theta_d, kLensK), theta, 1e-9);
  }
}

TEST(LensModel, ZeroCoefficientsAreEquidistant) {
  const std::array<double, 4> zero = {0, 0, 0, 0};
  EXPECT_DOUBLE_EQ(lens::KannalaBrandtRadius(0.7, zero), 0.7);
}

TEST(LensModel, FocalLengthPutsImageEdgesAtHalfFov) {
  projectairsim::LensSettings pinhole;
  EXPECT_NEAR(lens::FocalLengthPx(pinhole, 1280, 90.0), 640.0, 1e-9);

  projectairsim::LensSettings fisheye;
  fisheye.model = projectairsim::LensModel::kKannalaBrandt;
  // Equidistant: r = f theta, so 640 px at 60 degrees.
  EXPECT_NEAR(lens::FocalLengthPx(fisheye, 1280, 120.0), 640.0 / (kPi / 3),
              1e-9);
}

TEST(LensModel, PinholeSourceCoversTheFisheyeImage) {
  projectairsim::LensSettings fisheye;
  fisheye.model = projectairsim::LensModel::kKannalaBrandt;
  const auto source = lens::SourceForKannalaBrandt(fisheye, 1280, 720, 120.0);
  const double f = lens::FocalLengthPx(fisheye, 1280, 120.0);
  EXPECT_NEAR(source.focal_px, f, 1e-9);  // source_sampling 1
  EXPECT_GT(source.fov_degrees, 120.0);
  EXPECT_EQ(source.width % 2, 0u);
  EXPECT_EQ(source.height % 2, 0u);
  // The output's corner ray lands inside the source image.
  const double x = 639.5 / f, y = 359.5 / f, theta_d = std::hypot(x, y);
  const double scale = std::tan(theta_d) / theta_d * source.focal_px;
  EXPECT_LT(x * scale, source.width / 2.0);
  EXPECT_LT(y * scale, source.height / 2.0);
}

TEST(LensModel, SourceIsCappedAndIncidenceValidated) {
  projectairsim::LensSettings fisheye;
  fisheye.model = projectairsim::LensModel::kKannalaBrandt;
  fisheye.max_incidence_degrees = 88.0;
  const auto source = lens::SourceForKannalaBrandt(fisheye, 1280, 720, 170.0);
  EXPECT_LE(source.width, 4096u + 4u);
  fisheye.max_incidence_degrees = 95.0;
  EXPECT_THROW(lens::SourceForKannalaBrandt(fisheye, 1280, 720, 120.0),
               projectairsim::Error);
}

TEST(LensModel, PinholeSupersamplingRendersALargerImageOfTheSameView) {
  projectairsim::LensSettings pinhole;
  EXPECT_FALSE(lens::NeedsRemap(pinhole));
  pinhole.source_sampling = 2.0;
  EXPECT_TRUE(lens::NeedsRemap(pinhole));
  const auto source = lens::RenderSource(pinhole, 1280, 720, 90.0);
  EXPECT_EQ(source.width, 2560u);
  EXPECT_EQ(source.height, 1440u);
  EXPECT_DOUBLE_EQ(source.fov_degrees, 90.0);
  EXPECT_NEAR(source.focal_px, 1280.0, 1e-9);

  EXPECT_THROW(lens::RenderSource(pinhole, 2560, 1440, 90.0),
               projectairsim::Error);  // over 4096 px per side

  // Only 2 is an exact box filter; anything else throws.
  for (const double sampling : {0.0, 1.5, 3.0}) {
    pinhole.source_sampling = sampling;
    EXPECT_THROW(lens::RenderSource(pinhole, 1280, 720, 90.0),
                 projectairsim::Error);
  }
}
