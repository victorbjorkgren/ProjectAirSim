// Copyright (C) Microsoft Corporation.  
// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.

#ifndef CORE_SIM_INCLUDE_CORE_SIM_SENSORS_CAMERA_HPP_
#define CORE_SIM_INCLUDE_CORE_SIM_SENSORS_CAMERA_HPP_

#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "core_sim/math_utils.hpp"
#include "core_sim/message/camera_info_message.hpp"
#include "core_sim/message/image_message.hpp"
#include "core_sim/sensors/lens_model.hpp"
#include "core_sim/sensors/sensor.hpp"
#include "core_sim/transforms/transform.hpp"

namespace microsoft {
namespace projectairsim {

class ConfigJson;
class Logger;
class SensorImpl;
class TopicManager;
class ServiceManager;
class StateManager;

// ----------------------------------------------------------------------------

enum class ImageType : int {
  kScene = 0,
  kDepthPlanar = 1,
  kDepthPerspective = 2,
  kSegmentation = 3,
  kDepthVis = 4,
  kDisparityNormalized = 5,
  kSurfaceNormals = 6,
  kInfrared = 7,     // thermal (LWIR) camera, 8-bit display image
  kTemperature = 8,  // thermal camera, radiometric, uint16 centikelvin
  kCount  //  must be last
};        // enum class ImageType

//! Mirrors UE's EAutoExposureMethod
enum class ExposureMethod : int {
  kHistogram = 0,  //! Uses 64 bin histogram
  kBasic = 1,      //! Faster method; Computes value by downsampling
  kManual = 2,
  kMax = 3
};  // enum class ExposureMethod

// For bounding box settings
enum BoxAlignment {
  kAxis = 0,     // view for 2D, world for 3D
  kOriented = 1  // tightest, arbitrarily-oriented box
};               // enum BoxAlignment

NLOHMANN_JSON_SERIALIZE_ENUM(BoxAlignment, {
                                               {kAxis, "axis"},
                                               {kOriented, "oriented"},
                                           })

//! Sensor model of the thermal camera (image types kInfrared and kTemperature).
//! Both types come from one render and share these settings, which are read
//! from the "thermal" object of the kInfrared capture setting.
struct ThermalSettings {
  enum class Agc : int { kPercentile = 0, kHistogramEqualization = 1, kFixed = 2 };
  enum class Palette : int { kWhiteHot = 0, kBlackHot = 1, kIronbow = 2 };

  // Optics and detector
  float psf_sigma_px = 0.8f;          // Gaussian blur of the optics, in pixels
  float vignette = 0.12f;             // relative loss of signal in the corners
  float halo = 0.0f;                  // edge-sharpening overshoot of the camera's image processing (0 = none)
  // Noise: deterministic, from `seed` and the frame counter
  bool noise_enabled = true;
  uint32_t seed = 1234;
  float netd_kelvin = 0.05f;          // temporal noise, RMS in K at 300 K scene
  float fpn_gain_sigma = 0.004f;      // per-pixel gain spread after non-uniformity correction
  float fpn_offset_kelvin = 0.08f;    // per-pixel offset spread, K
  float fpn_column_kelvin = 0.05f;    // per-column stripe amplitude, K
  // Display
  Agc agc = Agc::kPercentile;
  float agc_low_percent = 1.0f;       // percentile mapped to black
  float agc_high_percent = 99.5f;     // percentile mapped to white
  float agc_min_span_kelvin = 6.0f;   // the display never stretches a smaller span than this
  float agc_smoothing = 0.9f;         // 0..1 weight of the previous frame's range (steady display)
  float agc_target_median = 0.42f;    // display level (0..1) the percentile AGC gives the scene median; 0 = linear stretch
  float fixed_min_celsius = -20.0f;   // Agc::kFixed span
  float fixed_max_celsius = 60.0f;
  Palette palette = Palette::kWhiteHot;
  // Environment: an environment can also set these from its own code or JSON
  std::string classes_file = "";      // optional path of the thermal class mapping JSON
};  // struct ThermalSettings

struct CaptureSettings {
  /*
  template <typename T>
  static constexpr T my_nan()
  {
      return std::numeric_limits<T>::quiet_NaN();
  }
  */
  int image_type = 0;
  bool capture_enabled = false;
  bool streaming_enabled = false;
  bool show_debug_plots = false;

  static constexpr float kSceneTargetGamma = 1.4f;

  unsigned int width = 256, height = 144;                       //  960 X 540
  float fov_degrees = std::numeric_limits<float>::quiet_NaN();  //  90.0f
  bool pixels_as_float = false;
  bool compress = true;

  int auto_exposure_method = static_cast<int>(ExposureMethod::kHistogram);
  float auto_exposure_speed =
      std::numeric_limits<float>::quiet_NaN();  // 100.0f;
  float auto_exposure_bias = std::numeric_limits<float>::quiet_NaN();  // 0;
  float auto_exposure_max_brightness =
      std::numeric_limits<float>::quiet_NaN();  // 0.64f;
  float auto_exposure_min_brightness =
      std::numeric_limits<float>::quiet_NaN();  // 0.03f;
  float auto_exposure_low_percent =
      std::numeric_limits<float>::quiet_NaN();  // 80.0f;
  float auto_exposure_high_percent =
      std::numeric_limits<float>::quiet_NaN();  // 98.3f;
  float auto_exposure_histogram_log_min =
      std::numeric_limits<float>::quiet_NaN();  // -8;
  float auto_exposure_histogram_log_max =
      std::numeric_limits<float>::quiet_NaN();  // 4;
  float motion_blur_amount = std::numeric_limits<float>::quiet_NaN();
  float target_gamma =
      2.5f;  // This would be reset to kSceneTargetGamma for scene as default
  int projection_mode = 0;  // ECameraProjectionMode::Perspective
  float ortho_width = std::numeric_limits<float>::quiet_NaN();
  float max_depth_meters = 1.0;  // for depthvis
  float depth_of_field_focal_meters = std::numeric_limits<float>::quiet_NaN();
  float depth_of_field_transition_region =
      std::numeric_limits<float>::quiet_NaN();
  float chromatic_aberration_intensity =
      std::numeric_limits<float>::quiet_NaN();
  // NaN keeps what the level's post-process volumes give the capture.
  float vignette_intensity = std::numeric_limits<float>::quiet_NaN();
  float lens_flare_intensity = std::numeric_limits<float>::quiet_NaN();
  LensSettings lens;  // "lens": pinhole (default) or kannala-brandt
  ThermalSettings thermal;  // "thermal": sensor model of image types 7 and 8
};  // struct CaptureSettings

struct NoiseSettings {
  int ImageType = 0;

  bool Enabled = false;

  float RandContrib = 0.2f;
  float RandSpeed = 100000.0f;
  float RandSize = 500.0f;
  float RandDensity = 2.0f;

  float HorzWaveContrib = 0.03f;
  float HorzWaveStrength = 0.08f;
  float HorzWaveVertSize = 1.0f;
  float HorzWaveScreenSize = 1.0f;

  float HorzNoiseLinesContrib = 1.0f;
  float HorzNoiseLinesDensityY = 0.01f;
  float HorzNoiseLinesDensityXY = 0.5f;

  float HorzDistortionContrib = 1.0f;
  float HorzDistortionStrength = 0.002f;
};  // struct NoiseSettings

struct GimbalSettings {
  bool is_gimbal_mounted = false;
  std::string gimbal_id = "";
  bool lock_roll = false;
  bool lock_pitch = false;
  bool lock_yaw = false;
};  // struct GimbalSettings

struct BBoxSettings {
  BoxAlignment alignment = BoxAlignment::kOriented;
};  // struct BBoxSettings

struct AnnotationSettings {
  std::unordered_set<std::string> object_ids;
  BBoxSettings bbox2D_settings;
  BBoxSettings bbox3D_settings;
  bool enabled = false;
  bool distance_filter_enabled = false;
  float distance_filter_range = 10.0;
};  // struct AnnotationSettings

struct PostProcessingModelSettings {
  bool enabled = false;
  std::string execution_provider = "cpu";
  std::string filepath = "";
  bool session_initialized = false;
};  // struct PostProcessingModelSettings

struct CameraSettings {
  Transform origin_setting;

  GimbalSettings gimbal_setting;
  float capture_interval = 5.0f;  // seconds
  std::vector<CaptureSettings> capture_settings;
  std::vector<NoiseSettings> noise_settings;
  AnnotationSettings annotation_settings;
  PostProcessingModelSettings post_process_model_settings;

  // Advanced settings
  const float sensor_width = 23.76f;  // mm
  float aperture = 2.0;               // f-stop: 1.4, 2, 2.8, 4, 5.6, 8...
  // TODO: add ISO, filmback, etc.

  CameraSettings() {
    origin_setting.translation_ = Vector3(0, 0, 0);
    origin_setting.rotation_ = Quaternion::Identity();
  }
};  // struct CameraSettings

// camera type

class Camera : public Sensor {
 public:
  Camera();

  const CameraSettings& GetCameraSettings() const;

  void BeginUpdate();

  void PublishImages(std::map<ImageType, ImageMessage>&& images);

  // AddRequestToCaptureQueue() and IsCaptureQueueFull() methods below are
  // public because the image compression/packaging/publishing after each
  // captured image is rendered is currently handled on the Unreal side using
  // its built-in async thread management and image compression library. If this
  // part of the image processing is moved inside sim camera, these methods
  // could be handled internally as well.
  void AddRequestToCaptureQueue(TimeNano time_stamp);

  bool IsCaptureQueueFull() const;

  void EndUpdate();

  bool IsPoseUpdatePending() const;

  bool IsSettingsUpdatePending() const;

  const std::string GetLookAtObject() const;

  const bool GetFrustumEnabled() const;

  const int GetFrustumCaptureType() const;

  Pose GetRay(ImageType image_type, const Vector2& image_position) const;

  bool SetPose(const Transform& new_pose, bool wait_for_pose_update);

  bool HasGimbal() const;

  const std::string& GetGimbalId() const;

  const Transform& GetDesiredPose() const;

  void MarkPoseUpdateAsCompleted();

  void MarkSettingsUpdateAsCompleted();

  const bool ResetCameraPose(bool wait_for_pose_update);

  bool HasSubscribers(int iimage_type) const;

 private:
  friend class Robot;
  friend class SensorImpl;

  Camera(const std::string& id, bool is_enabled, const std::string& parent_link,
         const Logger& logger, const TopicManager& topic_manager,
         const std::string& parent_topic_path,
         const ServiceManager& service_manager,
         const StateManager& state_manager);

  void Load(ConfigJson config_json) override;

  void Initialize(const Kinematics&, const Environment&) override;

  void Update(const TimeNano, const TimeNano) override;

  class Impl;
  class Loader;
};  // class Camera

}  // namespace projectairsim
}  // namespace microsoft

#endif  // CORE_SIM_INCLUDE_CORE_SIM_SENSORS_CAMERA_HPP_
