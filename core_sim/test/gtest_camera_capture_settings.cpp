// Copyright (C) Microsoft Corporation.  
// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.
// Tests for Camera Capture Settings

#include "core_sim/config_json.hpp"
#include "core_sim/logger.hpp"
#include "core_sim/sensors/camera.hpp"
#include "core_sim/sensors/sensor.hpp"
#include "core_sim/service_manager.hpp"
#include "gtest/gtest.h"
#include "state_manager.hpp"
#include "topic_manager.hpp"

using json = nlohmann::json;

namespace microsoft {
namespace projectairsim {

class Robot {  // : public ::testing::Test {
               // protected:
 public:
  static Camera MakeDefaultCamera() {
    const std::string& id = "TestCamID";
    const bool& is_enabled = true;
    const std::string& parent_link = "ParentLink1";
    auto logger_callback = [](const std::string& component, LogLevel level,
                              const std::string& message) {};
    Logger logger(logger_callback);
    const std::string& parent_topic_path = "/camera_test_topic";
    return Camera(id, is_enabled, parent_link, logger, TopicManager(logger),
                  parent_topic_path, ServiceManager(logger),
                  StateManager(logger));
  }

  static void LoadCamera(Camera& camera, ConfigJson config_json) {
    camera.Load(config_json);
  }

  static json GetDefaultCameraConfig() {
    json config = R"({
        "id": "ID123",
        "type": "camera",
        "enabled": true,
        "parent-link": "ParentLink",
        "capture-interval": 1.23,
        "capture-settings": [],
        "noise-settings": [],
        "gimbal": {},
        "origin": {}
    })"_json;
    return config;
  }
};

}  // namespace projectairsim
}  // namespace microsoft

namespace projectairsim = microsoft::projectairsim;

TEST(CameraCaptureSettings, SetsCaptureInterval) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  const auto& actual_camera_settings = camera.GetCameraSettings();
  EXPECT_FLOAT_EQ(actual_camera_settings.capture_interval, 1.23);
}

TEST(CameraCaptureSettings, EnablesCapture) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  const auto& actual_camera_settings = camera.GetCameraSettings();
  auto actual_capture_settings = actual_camera_settings.capture_settings;
  EXPECT_FLOAT_EQ(actual_capture_settings.at(2).capture_enabled, false);

  json capture_settings = R"( [ {
          "capture-enabled": true,
          "image-type": 2
      } ]
  )"_json;
  camera_settings["capture-settings"] = capture_settings;
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  const auto& new_actual_camera_settings = camera.GetCameraSettings();
  actual_capture_settings = new_actual_camera_settings.capture_settings;
  EXPECT_FLOAT_EQ(actual_capture_settings.at(2).capture_enabled, true);
}

TEST(CameraCaptureSettings, EnablesAutoExposureMethodParam) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  const auto& actual_camera_settings = camera.GetCameraSettings();
  auto actual_capture_settings = actual_camera_settings.capture_settings;
  EXPECT_FLOAT_EQ(actual_capture_settings.at(2).auto_exposure_method, 0);

  json capture_settings = R"( [ {
          "image-type": 2,
          "auto-exposure-method": 1
      } ]
  )"_json;
  camera_settings["capture-settings"] = capture_settings;
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  const auto& new_actual_camera_settings = camera.GetCameraSettings();
  actual_capture_settings = new_actual_camera_settings.capture_settings;
  EXPECT_FLOAT_EQ(actual_capture_settings.at(2).auto_exposure_method, 1);
}

TEST(CameraCaptureSettings, EnablesWidthParam) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  json capture_settings = R"( [ {
          "image-type": 2,
          "width": 123
      } ]
  )"_json;
  camera_settings["capture-settings"] = capture_settings;
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  const auto& actual_camera_settings = camera.GetCameraSettings();
  auto actual_capture_settings = actual_camera_settings.capture_settings;
  EXPECT_FLOAT_EQ(actual_capture_settings.at(2).width, 123);
}

TEST(CameraCaptureSettings, EnablesHeightParam) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  json capture_settings = R"( [ {
          "image-type": 2,
          "height": 123
      } ]
  )"_json;
  camera_settings["capture-settings"] = capture_settings;
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  const auto& actual_camera_settings = camera.GetCameraSettings();
  auto actual_capture_settings = actual_camera_settings.capture_settings;
  EXPECT_FLOAT_EQ(actual_capture_settings.at(2).height, 123);
}

TEST(CameraCaptureSettings, EnablesFovDegreesParam) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  json capture_settings = R"( [ {
          "image-type": 2,
          "fov-degrees": 123
      } ]
  )"_json;
  camera_settings["capture-settings"] = capture_settings;
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  const auto& actual_camera_settings = camera.GetCameraSettings();
  auto actual_capture_settings = actual_camera_settings.capture_settings;
  EXPECT_FLOAT_EQ(actual_capture_settings.at(2).fov_degrees, 123);
}

TEST(CameraCaptureSettings, EnablesAutoExposureSpeedParam) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  json capture_settings = R"( [ {
          "image-type": 2,
          "auto-exposure-speed": 1.23
      } ]
  )"_json;
  camera_settings["capture-settings"] = capture_settings;
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  const auto& actual_camera_settings = camera.GetCameraSettings();
  auto actual_capture_settings = actual_camera_settings.capture_settings;
  EXPECT_FLOAT_EQ(actual_capture_settings.at(2).auto_exposure_speed, 1.23);
}

TEST(CameraCaptureSettings, EnablesAutoExposureBiasParam) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  json capture_settings = R"( [ {
          "image-type": 2,
          "auto-exposure-bias": 0.123
      } ]
  )"_json;
  camera_settings["capture-settings"] = capture_settings;
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  const auto& actual_camera_settings = camera.GetCameraSettings();
  auto actual_capture_settings = actual_camera_settings.capture_settings;
  EXPECT_FLOAT_EQ(actual_capture_settings.at(2).auto_exposure_bias, 0.123);
}

TEST(CameraCaptureSettings, EnablesAutoExposureMaxBrightnessParam) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  json capture_settings = R"( [ {
          "image-type": 2,
          "auto-exposure-max-brightness": 1.23
      } ]
  )"_json;
  camera_settings["capture-settings"] = capture_settings;
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  const auto& actual_camera_settings = camera.GetCameraSettings();
  auto actual_capture_settings = actual_camera_settings.capture_settings;
  EXPECT_FLOAT_EQ(actual_capture_settings.at(2).auto_exposure_max_brightness,
                  1.23);
}

TEST(CameraCaptureSettings, EnablesAutoExposureMinBrightnessParam) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  json capture_settings = R"( [ {
          "image-type": 2,
          "auto-exposure-min-brightness": 0.123
      } ]
  )"_json;
  camera_settings["capture-settings"] = capture_settings;
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  const auto& actual_camera_settings = camera.GetCameraSettings();
  auto actual_capture_settings = actual_camera_settings.capture_settings;
  EXPECT_FLOAT_EQ(actual_capture_settings.at(2).auto_exposure_min_brightness,
                  0.123);
}

TEST(CameraCaptureSettings, EnablesAutoExposureLowPercentParam) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  json capture_settings = R"( [ {
          "image-type": 2,
          "auto-exposure-low-percent": 0.123
      } ]
  )"_json;
  camera_settings["capture-settings"] = capture_settings;
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  const auto& actual_camera_settings = camera.GetCameraSettings();
  auto actual_capture_settings = actual_camera_settings.capture_settings;
  EXPECT_FLOAT_EQ(actual_capture_settings.at(2).auto_exposure_low_percent,
                  0.123);
}

TEST(CameraCaptureSettings, EnablesAutoExposureHighPercentParam) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  json capture_settings = R"( [ {
          "image-type": 2,
          "auto-exposure-high-percent": 0.99
      } ]
  )"_json;
  camera_settings["capture-settings"] = capture_settings;
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  const auto& actual_camera_settings = camera.GetCameraSettings();
  auto actual_capture_settings = actual_camera_settings.capture_settings;
  EXPECT_FLOAT_EQ(actual_capture_settings.at(2).auto_exposure_high_percent,
                  0.99);
}

TEST(CameraCaptureSettings, EnablesAutoExposureHistogramLogMinParam) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  json capture_settings = R"( [ {
          "image-type": 2,
          "auto-exposure-histogram-log-min": -12.3
      } ]
  )"_json;
  camera_settings["capture-settings"] = capture_settings;
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  const auto& actual_camera_settings = camera.GetCameraSettings();
  auto actual_capture_settings = actual_camera_settings.capture_settings;
  EXPECT_FLOAT_EQ(actual_capture_settings.at(2).auto_exposure_histogram_log_min,
                  -12.3);
}

TEST(CameraCaptureSettings, EnablesAutoExposureHistogramLogMaxParam) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  json capture_settings = R"( [ {
          "image-type": 2,
          "auto-exposure-histogram-log-max": 12.3
      } ]
  )"_json;
  camera_settings["capture-settings"] = capture_settings;
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  const auto& actual_camera_settings = camera.GetCameraSettings();
  auto actual_capture_settings = actual_camera_settings.capture_settings;
  EXPECT_FLOAT_EQ(actual_capture_settings.at(2).auto_exposure_histogram_log_max,
                  12.3);
}

TEST(CameraCaptureSettings, EnablesMotionBlurParam) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  json capture_settings = R"( [ {
          "image-type": 2,
          "motion-blur-amount": 1.23
      } ]
  )"_json;
  camera_settings["capture-settings"] = capture_settings;
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  const auto& actual_camera_settings = camera.GetCameraSettings();
  auto actual_capture_settings = actual_camera_settings.capture_settings;
  EXPECT_FLOAT_EQ(actual_capture_settings.at(2).motion_blur_amount, 1.23);
}

TEST(CameraCaptureSettings, LoadsKannalaBrandtLens) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  json capture_settings = R"( [ {
          "image-type": 0,
          "lens": {"model": "kannala-brandt", "distortion": [0.1, -0.02, 0.003, -0.0004],
                   "max-incidence-degrees": 75, "source-sampling": 0.8}
      } ]
  )"_json;
  camera_settings["capture-settings"] = capture_settings;
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  const auto& lens = camera.GetCameraSettings().capture_settings.at(0).lens;
  EXPECT_EQ(lens.model, projectairsim::LensModel::kKannalaBrandt);
  EXPECT_DOUBLE_EQ(lens.distortion[0], 0.1);
  EXPECT_DOUBLE_EQ(lens.distortion[3], -0.0004);
  EXPECT_DOUBLE_EQ(lens.max_incidence_degrees, 75.0);
  EXPECT_DOUBLE_EQ(lens.source_sampling, 0.8);
}

TEST(CameraCaptureSettings, DefaultsToPinholeLens) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  json capture_settings = R"( [ { "image-type": 0 } ] )"_json;
  camera_settings["capture-settings"] = capture_settings;
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  EXPECT_EQ(camera.GetCameraSettings().capture_settings.at(0).lens.model,
            projectairsim::LensModel::kPinhole);
}

TEST(CameraCaptureSettings, LoadsThermalSettingsAndSharesThemWithTemperature) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  json capture_settings = R"( [ {
          "image-type": 8, "capture-enabled": true, "width": 640, "height": 512, "fov-degrees": 56
      }, {
          "image-type": 7, "capture-enabled": true, "width": 640, "height": 512, "fov-degrees": 56,
          "thermal": {"palette": "ironbow", "agc": "histogram-equalization", "seed": 7,
                      "netd-kelvin": 0.03, "noise-enabled": false, "classes-file": "c.json"}
      } ]
  )"_json;
  camera_settings["capture-settings"] = capture_settings;
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  const auto& all = camera.GetCameraSettings().capture_settings;
  const auto& t = all.at(7).thermal;
  EXPECT_EQ(t.palette, projectairsim::ThermalSettings::Palette::kIronbow);
  EXPECT_EQ(t.agc, projectairsim::ThermalSettings::Agc::kHistogramEqualization);
  EXPECT_EQ(t.seed, 7u);
  EXPECT_FLOAT_EQ(t.netd_kelvin, 0.03f);
  EXPECT_FALSE(t.noise_enabled);
  EXPECT_EQ(t.classes_file, "c.json");
  EXPECT_EQ(all.at(8).thermal.seed, 7u);  // the sensor model is shared
}

TEST(CameraCaptureSettings, TemperatureAloneGivesTheInfraredEntryItsSettings) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  json capture_settings = R"( [ {
          "image-type": 8, "capture-enabled": true, "width": 320, "height": 256, "fov-degrees": 40
      } ] )"_json;
  camera_settings["capture-settings"] = capture_settings;
  projectairsim::Robot::LoadCamera(camera, camera_settings);
  const auto& all = camera.GetCameraSettings().capture_settings;
  EXPECT_EQ(all.at(7).width, 320);
  EXPECT_FALSE(all.at(7).capture_enabled);
  EXPECT_TRUE(all.at(8).capture_enabled);
}

TEST(CameraCaptureSettings, ThermalRefusesAFisheyeLens) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  json capture_settings = R"( [ {
          "image-type": 7, "lens": {"model": "kannala-brandt", "distortion": [0, 0, 0, 0]}
      } ] )"_json;
  camera_settings["capture-settings"] = capture_settings;
  EXPECT_ANY_THROW(projectairsim::Robot::LoadCamera(camera, camera_settings));
}

TEST(CameraCaptureSettings, ThermalRefusesASupersampledPinhole) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  json capture_settings = R"( [ {
          "image-type": 7, "lens": {"model": "pinhole", "source-sampling": 2}
      } ] )"_json;
  camera_settings["capture-settings"] = capture_settings;
  EXPECT_ANY_THROW(projectairsim::Robot::LoadCamera(camera, camera_settings));
}

TEST(CameraCaptureSettings, ThermalRejectsAnUnknownPalette) {
  auto camera = projectairsim::Robot::MakeDefaultCamera();
  auto camera_settings = projectairsim::Robot::GetDefaultCameraConfig();
  json capture_settings = R"( [ {
          "image-type": 7, "thermal": {"palette": "rainbow"}
      } ] )"_json;
  camera_settings["capture-settings"] = capture_settings;
  EXPECT_ANY_THROW(projectairsim::Robot::LoadCamera(camera, camera_settings));
}
