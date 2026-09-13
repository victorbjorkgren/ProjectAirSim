// Copyright (C) Microsoft Corporation.
// Copyright (C) 2025 IAMAI CONSULTING CORP

// MIT License. All rights reserved.

#ifndef PHYSICS_TEST_TEST_DATA_PHYSICS_TEST_ROVER_CONFIG_HPP_
#define PHYSICS_TEST_TEST_DATA_PHYSICS_TEST_ROVER_CONFIG_HPP_

// Rover scene configs, modeled on
// Autopilot/cobra_flex/sim/config/robot_cobra_flex_sim.jsonc (this repo's
// real differential-drive rover, no steering wheels), used by
// gtest_fast_physics.cpp to test FastPhysicsModel::CalcNextKinematicsWithWheels()'s
// branches:
//
// - physics_test_diffdrive_rover_config: all four wheels "steering": false,
//   exercising the differential-drive yaw model.
// - physics_test_ackermann_rover_config: identical geometry, but
//   Wheel_FL_actuator has "steering": true, exercising the pre-existing
//   Ackermann steering-angle yaw model to confirm it is untouched by the
//   differential-drive branch.
// - physics_test_diffdrive_rover_engine_disabled_config: same as
//   physics_test_diffdrive_rover_config, but one wheel has "engine": false,
//   exercising the differential-drive branch's engine-connected filter.
// - physics_test_diffdrive_rover_centerline_wheel_config: a 3-wheel variant
//   with one wheel at Y = 0.0, exercising the centerline-wheel handling in
//   the differential-drive branch's forward-speed average.
// - physics_test_near_zero_trackwidth_rover_config: same 4-wheel layout,
//   but Y offsets differ by only 1e-6 m, exercising the epsilon on the
//   construction-time track_width_ guard.
//
// The two 4-wheel configs' wheel origins (X = +-0.6, Y = +-0.5) give
// rover_length_ = 1.2 m and track_width_ = 1.0 m via
// FastPhysicsBody::InitializeFastPhysicsBody()'s existing wheels[0]-vs-[2]
// (X) / wheels[0]-vs-[1] (Y) pattern -- see fast_physics.cpp.
constexpr const char* physics_test_diffdrive_rover_config = R"(
{
  "id": "SceneTestDiffDriveRover",
  "actors": [
    {
      "type": "robot",
      "name": "Rover1",
      "origin": {
        "xyz": "0.0 0.0 0.0",
        "rpy-deg": "0 0 0"
      },
      "robot-config": {
        "physics-type": "fast-physics",
        "links": [
          {
            "name": "Frame",
            "inertial": {
              "mass": 5.0,
              "inertia": {
                "type": "geometry",
                "geometry": { "box": { "size": "0.35 0.30 0.15" } }
              }
            },
            "visual": {
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Body" }
            }
          },
          {
            "name": "Front_Left",
            "inertial": {
              "origin": { "xyz": "0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          },
          {
            "name": "Front_Right",
            "inertial": {
              "origin": { "xyz": "0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          },
          {
            "name": "Rear_Left",
            "inertial": {
              "origin": { "xyz": "-0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "-0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          },
          {
            "name": "Rear_Right",
            "inertial": {
              "origin": { "xyz": "-0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "-0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          }
        ],
        "joints": [
          { "id": "Frame_Wheel_FL", "type": "fixed", "parent-link": "Frame", "child-link": "Front_Left", "axis": "0 0 1" },
          { "id": "Frame_Wheel_FR", "type": "fixed", "parent-link": "Frame", "child-link": "Front_Right", "axis": "0 0 1" },
          { "id": "Frame_Wheel_RL", "type": "fixed", "parent-link": "Frame", "child-link": "Rear_Left", "axis": "0 0 1" },
          { "id": "Frame_Wheel_RR", "type": "fixed", "parent-link": "Frame", "child-link": "Rear_Right", "axis": "0 0 1" }
        ],
        "controller": {
          "id": "Manual_Controller",
          "type": "manual-controller-api",
          "manual-controller-api-settings": {
            "actuator-order": [
              { "id": "Wheel_FL_actuator" },
              { "id": "Wheel_FR_actuator" },
              { "id": "Wheel_RL_actuator" },
              { "id": "Wheel_RR_actuator" }
            ]
          }
        },
        "actuators": [
          {
            "name": "Wheel_FL_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Front_Left",
            "origin": { "xyz": "0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          },
          {
            "name": "Wheel_FR_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Front_Right",
            "origin": { "xyz": "0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          },
          {
            "name": "Wheel_RL_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Rear_Left",
            "origin": { "xyz": "-0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          },
          {
            "name": "Wheel_RR_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Rear_Right",
            "origin": { "xyz": "-0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          }
        ],
        "sensors": []
      }
    }
  ],
  "clock": {
    "type": "steppable",
    "step-ns": 3000000,
    "real-time-update-rate": 3000000,
    "pause-on-start": false
  },
  "home-geo-point": {
    "latitude": 47.641468,
    "longitude": -122.140165,
    "altitude": 122.0
  },
  "segmentation": {
    "initialize-ids": true,
    "ignore-existing": false,
    "use-owner-name": true
  }
}
)";

// Same rover, but Wheel_FL_actuator is steering-capable (Ackermann-style),
// so FastPhysicsModel::CalcNextKinematicsWithWheels() must take the
// pre-existing Ackermann branch, unmodified, instead of the new
// differential-drive branch.
constexpr const char* physics_test_ackermann_rover_config = R"(
{
  "id": "SceneTestAckermannRover",
  "actors": [
    {
      "type": "robot",
      "name": "Rover1",
      "origin": {
        "xyz": "0.0 0.0 0.0",
        "rpy-deg": "0 0 0"
      },
      "robot-config": {
        "physics-type": "fast-physics",
        "links": [
          {
            "name": "Frame",
            "inertial": {
              "mass": 5.0,
              "inertia": {
                "type": "geometry",
                "geometry": { "box": { "size": "0.35 0.30 0.15" } }
              }
            },
            "visual": {
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Body" }
            }
          },
          {
            "name": "Front_Left",
            "inertial": {
              "origin": { "xyz": "0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          },
          {
            "name": "Front_Right",
            "inertial": {
              "origin": { "xyz": "0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          },
          {
            "name": "Rear_Left",
            "inertial": {
              "origin": { "xyz": "-0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "-0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          },
          {
            "name": "Rear_Right",
            "inertial": {
              "origin": { "xyz": "-0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "-0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          }
        ],
        "joints": [
          { "id": "Frame_Wheel_FL", "type": "fixed", "parent-link": "Frame", "child-link": "Front_Left", "axis": "0 0 1" },
          { "id": "Frame_Wheel_FR", "type": "fixed", "parent-link": "Frame", "child-link": "Front_Right", "axis": "0 0 1" },
          { "id": "Frame_Wheel_RL", "type": "fixed", "parent-link": "Frame", "child-link": "Rear_Left", "axis": "0 0 1" },
          { "id": "Frame_Wheel_RR", "type": "fixed", "parent-link": "Frame", "child-link": "Rear_Right", "axis": "0 0 1" }
        ],
        "controller": {
          "id": "Manual_Controller",
          "type": "manual-controller-api",
          "manual-controller-api-settings": {
            "actuator-order": [
              { "id": "Wheel_FL_actuator" },
              { "id": "Wheel_FR_actuator" },
              { "id": "Wheel_RL_actuator" },
              { "id": "Wheel_RR_actuator" }
            ]
          }
        },
        "actuators": [
          {
            "name": "Wheel_FL_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Front_Left",
            "origin": { "xyz": "0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 1.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": true,
              "brake": false,
              "smoothing-tc": 0.0
            }
          },
          {
            "name": "Wheel_FR_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Front_Right",
            "origin": { "xyz": "0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          },
          {
            "name": "Wheel_RL_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Rear_Left",
            "origin": { "xyz": "-0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          },
          {
            "name": "Wheel_RR_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Rear_Right",
            "origin": { "xyz": "-0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          }
        ],
        "sensors": []
      }
    }
  ],
  "clock": {
    "type": "steppable",
    "step-ns": 3000000,
    "real-time-update-rate": 3000000,
    "pause-on-start": false
  },
  "home-geo-point": {
    "latitude": 47.641468,
    "longitude": -122.140165,
    "altitude": 122.0
  },
  "segmentation": {
    "initialize-ids": true,
    "ignore-existing": false,
    "use-owner-name": true
  }
}
)";

// Same rover as physics_test_diffdrive_rover_config (no steering wheels),
// but all four wheels share Y = 0.0 instead of +-0.5. This makes
// track_width_ resolve to 0 via both the primary (wheels[0] vs [1]) and
// fallback (wheels[0] vs [2]) Y-offset differences in
// FastPhysicsBody::InitializeFastPhysicsBody() -- the misconfigured-geometry
// case that must throw rather than silently zero the differential-drive
// yaw rate (issue #633).
constexpr const char* physics_test_degenerate_trackwidth_rover_config = R"(
{
  "id": "SceneTestDegenerateTrackWidthRover",
  "actors": [
    {
      "type": "robot",
      "name": "Rover1",
      "origin": {
        "xyz": "0.0 0.0 0.0",
        "rpy-deg": "0 0 0"
      },
      "robot-config": {
        "physics-type": "fast-physics",
        "links": [
          {
            "name": "Frame",
            "inertial": {
              "mass": 5.0,
              "inertia": {
                "type": "geometry",
                "geometry": { "box": { "size": "0.35 0.30 0.15" } }
              }
            },
            "visual": {
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Body" }
            }
          },
          {
            "name": "Front_Left",
            "inertial": {
              "origin": { "xyz": "0.6 0.0 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "0.6 0.0 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          },
          {
            "name": "Front_Right",
            "inertial": {
              "origin": { "xyz": "0.6 0.0 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "0.6 0.0 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          },
          {
            "name": "Rear_Left",
            "inertial": {
              "origin": { "xyz": "-0.6 0.0 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "-0.6 0.0 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          },
          {
            "name": "Rear_Right",
            "inertial": {
              "origin": { "xyz": "-0.6 0.0 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "-0.6 0.0 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          }
        ],
        "joints": [
          { "id": "Frame_Wheel_FL", "type": "fixed", "parent-link": "Frame", "child-link": "Front_Left", "axis": "0 0 1" },
          { "id": "Frame_Wheel_FR", "type": "fixed", "parent-link": "Frame", "child-link": "Front_Right", "axis": "0 0 1" },
          { "id": "Frame_Wheel_RL", "type": "fixed", "parent-link": "Frame", "child-link": "Rear_Left", "axis": "0 0 1" },
          { "id": "Frame_Wheel_RR", "type": "fixed", "parent-link": "Frame", "child-link": "Rear_Right", "axis": "0 0 1" }
        ],
        "controller": {
          "id": "Manual_Controller",
          "type": "manual-controller-api",
          "manual-controller-api-settings": {
            "actuator-order": [
              { "id": "Wheel_FL_actuator" },
              { "id": "Wheel_FR_actuator" },
              { "id": "Wheel_RL_actuator" },
              { "id": "Wheel_RR_actuator" }
            ]
          }
        },
        "actuators": [
          {
            "name": "Wheel_FL_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Front_Left",
            "origin": { "xyz": "0.6 0.0 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          },
          {
            "name": "Wheel_FR_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Front_Right",
            "origin": { "xyz": "0.6 0.0 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          },
          {
            "name": "Wheel_RL_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Rear_Left",
            "origin": { "xyz": "-0.6 0.0 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          },
          {
            "name": "Wheel_RR_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Rear_Right",
            "origin": { "xyz": "-0.6 0.0 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          }
        ],
        "sensors": []
      }
    }
  ],
  "clock": {
    "type": "steppable",
    "step-ns": 3000000,
    "real-time-update-rate": 3000000,
    "pause-on-start": false
  },
  "home-geo-point": {
    "latitude": 47.641468,
    "longitude": -122.140165,
    "altitude": 122.0
  },
  "segmentation": {
    "initialize-ids": true,
    "ignore-existing": false,
    "use-owner-name": true
  }
}
)";

// A non-steering rover with only two wheels, sharing Y = 0.0. With fewer
// than three wheels, FastPhysicsBody::InitializeFastPhysicsBody()'s
// track_width_ fallback (which reads wheels[2] when the primary wheels[0]
// vs [1] difference is 0) must not index past the end of the wheels vector.
// track_width_ should end up 0 same as the four-wheel degenerate case above,
// not read out-of-bounds memory.
constexpr const char* physics_test_two_wheel_rover_config = R"(
{
  "id": "SceneTestTwoWheelRover",
  "actors": [
    {
      "type": "robot",
      "name": "Rover1",
      "origin": {
        "xyz": "0.0 0.0 0.0",
        "rpy-deg": "0 0 0"
      },
      "robot-config": {
        "physics-type": "fast-physics",
        "links": [
          {
            "name": "Frame",
            "inertial": {
              "mass": 5.0,
              "inertia": {
                "type": "geometry",
                "geometry": { "box": { "size": "0.35 0.30 0.15" } }
              }
            },
            "visual": {
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Body" }
            }
          },
          {
            "name": "Left",
            "inertial": {
              "origin": { "xyz": "0.6 0.0 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "0.6 0.0 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          },
          {
            "name": "Right",
            "inertial": {
              "origin": { "xyz": "-0.6 0.0 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "-0.6 0.0 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          }
        ],
        "joints": [
          { "id": "Frame_Wheel_L", "type": "fixed", "parent-link": "Frame", "child-link": "Left", "axis": "0 0 1" },
          { "id": "Frame_Wheel_R", "type": "fixed", "parent-link": "Frame", "child-link": "Right", "axis": "0 0 1" }
        ],
        "controller": {
          "id": "Manual_Controller",
          "type": "manual-controller-api",
          "manual-controller-api-settings": {
            "actuator-order": [
              { "id": "Wheel_L_actuator" },
              { "id": "Wheel_R_actuator" }
            ]
          }
        },
        "actuators": [
          {
            "name": "Wheel_L_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Left",
            "origin": { "xyz": "0.6 0.0 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          },
          {
            "name": "Wheel_R_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Right",
            "origin": { "xyz": "-0.6 0.0 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          }
        ],
        "sensors": []
      }
    }
  ],
  "clock": {
    "type": "steppable",
    "step-ns": 3000000,
    "real-time-update-rate": 3000000,
    "pause-on-start": false
  },
  "home-geo-point": {
    "latitude": 47.641468,
    "longitude": -122.140165,
    "altitude": 122.0
  },
  "segmentation": {
    "initialize-ids": true,
    "ignore-existing": false,
    "use-owner-name": true
  }
}
)";

// Same rover as physics_test_diffdrive_rover_config, but Wheel_RR_actuator
// (Y = +0.5, the right side) has "engine": false. Used to confirm that
// CalcNextKinematicsWithWheels()'s differential-drive branch reads the
// config-loaded GetWheelSettings().engine_connected_ and excludes this
// wheel from the right-side speed average entirely, rather than folding it
// in via the always-true Wheel::IsEngineConnected() (see the fix's comment
// in fast_physics.cpp).
constexpr const char* physics_test_diffdrive_rover_engine_disabled_config = R"(
{
  "id": "SceneTestDiffDriveRoverEngineDisabled",
  "actors": [
    {
      "type": "robot",
      "name": "Rover1",
      "origin": {
        "xyz": "0.0 0.0 0.0",
        "rpy-deg": "0 0 0"
      },
      "robot-config": {
        "physics-type": "fast-physics",
        "links": [
          {
            "name": "Frame",
            "inertial": {
              "mass": 5.0,
              "inertia": {
                "type": "geometry",
                "geometry": { "box": { "size": "0.35 0.30 0.15" } }
              }
            },
            "visual": {
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Body" }
            }
          },
          {
            "name": "Front_Left",
            "inertial": {
              "origin": { "xyz": "0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          },
          {
            "name": "Front_Right",
            "inertial": {
              "origin": { "xyz": "0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          },
          {
            "name": "Rear_Left",
            "inertial": {
              "origin": { "xyz": "-0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "-0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          },
          {
            "name": "Rear_Right",
            "inertial": {
              "origin": { "xyz": "-0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "-0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          }
        ],
        "joints": [
          { "id": "Frame_Wheel_FL", "type": "fixed", "parent-link": "Frame", "child-link": "Front_Left", "axis": "0 0 1" },
          { "id": "Frame_Wheel_FR", "type": "fixed", "parent-link": "Frame", "child-link": "Front_Right", "axis": "0 0 1" },
          { "id": "Frame_Wheel_RL", "type": "fixed", "parent-link": "Frame", "child-link": "Rear_Left", "axis": "0 0 1" },
          { "id": "Frame_Wheel_RR", "type": "fixed", "parent-link": "Frame", "child-link": "Rear_Right", "axis": "0 0 1" }
        ],
        "controller": {
          "id": "Manual_Controller",
          "type": "manual-controller-api",
          "manual-controller-api-settings": {
            "actuator-order": [
              { "id": "Wheel_FL_actuator" },
              { "id": "Wheel_FR_actuator" },
              { "id": "Wheel_RL_actuator" },
              { "id": "Wheel_RR_actuator" }
            ]
          }
        },
        "actuators": [
          {
            "name": "Wheel_FL_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Front_Left",
            "origin": { "xyz": "0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          },
          {
            "name": "Wheel_FR_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Front_Right",
            "origin": { "xyz": "0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          },
          {
            "name": "Wheel_RL_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Rear_Left",
            "origin": { "xyz": "-0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          },
          {
            "name": "Wheel_RR_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Rear_Right",
            "origin": { "xyz": "-0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": false,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          }
        ],
        "sensors": []
      }
    }
  ],
  "clock": {
    "type": "steppable",
    "step-ns": 3000000,
    "real-time-update-rate": 3000000,
    "pause-on-start": false
  },
  "home-geo-point": {
    "latitude": 47.641468,
    "longitude": -122.140165,
    "altitude": 122.0
  },
  "segmentation": {
    "initialize-ids": true,
    "ignore-existing": false,
    "use-owner-name": true
  }
}
)";

// A non-steering, 3-wheel rover: Left (Y = -0.5) and Right (Y = +0.5) give
// the same track_width_ = 1.0 m as the 4-wheel configs above (via the
// wheels[0]-vs-[1] Y-offset pattern), plus a third wheel exactly on the
// centerline (Y = 0.0). Used to confirm that
// CalcNextKinematicsWithWheels()'s differential-drive branch folds the
// centerline wheel into forward speed without letting it skew the
// left/right yaw-rate asymmetry (issue #633 round-3 finding).
constexpr const char* physics_test_diffdrive_rover_centerline_wheel_config = R"(
{
  "id": "SceneTestDiffDriveRoverCenterlineWheel",
  "actors": [
    {
      "type": "robot",
      "name": "Rover1",
      "origin": {
        "xyz": "0.0 0.0 0.0",
        "rpy-deg": "0 0 0"
      },
      "robot-config": {
        "physics-type": "fast-physics",
        "links": [
          {
            "name": "Frame",
            "inertial": {
              "mass": 5.0,
              "inertia": {
                "type": "geometry",
                "geometry": { "box": { "size": "0.35 0.30 0.15" } }
              }
            },
            "visual": {
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Body" }
            }
          },
          {
            "name": "Left",
            "inertial": {
              "origin": { "xyz": "0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          },
          {
            "name": "Right",
            "inertial": {
              "origin": { "xyz": "-0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "-0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          },
          {
            "name": "Center",
            "inertial": {
              "origin": { "xyz": "0.0 0.0 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "0.0 0.0 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          }
        ],
        "joints": [
          { "id": "Frame_Wheel_L", "type": "fixed", "parent-link": "Frame", "child-link": "Left", "axis": "0 0 1" },
          { "id": "Frame_Wheel_R", "type": "fixed", "parent-link": "Frame", "child-link": "Right", "axis": "0 0 1" },
          { "id": "Frame_Wheel_C", "type": "fixed", "parent-link": "Frame", "child-link": "Center", "axis": "0 0 1" }
        ],
        "controller": {
          "id": "Manual_Controller",
          "type": "manual-controller-api",
          "manual-controller-api-settings": {
            "actuator-order": [
              { "id": "Wheel_L_actuator" },
              { "id": "Wheel_R_actuator" },
              { "id": "Wheel_C_actuator" }
            ]
          }
        },
        "actuators": [
          {
            "name": "Wheel_L_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Left",
            "origin": { "xyz": "0.6 -0.5 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          },
          {
            "name": "Wheel_R_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Right",
            "origin": { "xyz": "-0.6 0.5 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          },
          {
            "name": "Wheel_C_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Center",
            "origin": { "xyz": "0.0 0.0 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          }
        ],
        "sensors": []
      }
    }
  ],
  "clock": {
    "type": "steppable",
    "step-ns": 3000000,
    "real-time-update-rate": 3000000,
    "pause-on-start": false
  },
  "home-geo-point": {
    "latitude": 47.641468,
    "longitude": -122.140165,
    "altitude": 122.0
  },
  "segmentation": {
    "initialize-ids": true,
    "ignore-existing": false,
    "use-owner-name": true
  }
}
)";

// Same layout as physics_test_diffdrive_rover_config, but Y = +-0.0000005
// instead of +-0.5, giving track_width_ = 1e-6 -- positive, so it passed
// the pre-epsilon "<= 0" guard, but far below any plausible physical
// left/right wheel separation. Construction must still reject it (issue
// #633 round-3 finding): otherwise CalcNextKinematicsWithWheels()'s
// (v_left - v_right) / track_width_ would divide by that near-zero value
// every tick.
constexpr const char* physics_test_near_zero_trackwidth_rover_config = R"(
{
  "id": "SceneTestNearZeroTrackWidthRover",
  "actors": [
    {
      "type": "robot",
      "name": "Rover1",
      "origin": {
        "xyz": "0.0 0.0 0.0",
        "rpy-deg": "0 0 0"
      },
      "robot-config": {
        "physics-type": "fast-physics",
        "links": [
          {
            "name": "Frame",
            "inertial": {
              "mass": 5.0,
              "inertia": {
                "type": "geometry",
                "geometry": { "box": { "size": "0.35 0.30 0.15" } }
              }
            },
            "visual": {
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Body" }
            }
          },
          {
            "name": "Front_Left",
            "inertial": {
              "origin": { "xyz": "0.6 -0.0000005 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "0.6 -0.0000005 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          },
          {
            "name": "Front_Right",
            "inertial": {
              "origin": { "xyz": "0.6 0.0000005 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "0.6 0.0000005 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          },
          {
            "name": "Rear_Left",
            "inertial": {
              "origin": { "xyz": "-0.6 -0.0000005 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "-0.6 -0.0000005 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          },
          {
            "name": "Rear_Right",
            "inertial": {
              "origin": { "xyz": "-0.6 0.0000005 -0.3", "rpy-deg": "0 0 0" },
              "mass": 0.055,
              "inertia": { "type": "geometry" }
            },
            "visual": {
              "origin": { "xyz": "-0.6 0.0000005 -0.3", "rpy-deg": "0 0 0" },
              "geometry": { "type": "unreal_mesh", "name": "/Rover/OffroadCar/SM_Offroad_Tire" }
            }
          }
        ],
        "joints": [
          { "id": "Frame_Wheel_FL", "type": "fixed", "parent-link": "Frame", "child-link": "Front_Left", "axis": "0 0 1" },
          { "id": "Frame_Wheel_FR", "type": "fixed", "parent-link": "Frame", "child-link": "Front_Right", "axis": "0 0 1" },
          { "id": "Frame_Wheel_RL", "type": "fixed", "parent-link": "Frame", "child-link": "Rear_Left", "axis": "0 0 1" },
          { "id": "Frame_Wheel_RR", "type": "fixed", "parent-link": "Frame", "child-link": "Rear_Right", "axis": "0 0 1" }
        ],
        "controller": {
          "id": "Manual_Controller",
          "type": "manual-controller-api",
          "manual-controller-api-settings": {
            "actuator-order": [
              { "id": "Wheel_FL_actuator" },
              { "id": "Wheel_FR_actuator" },
              { "id": "Wheel_RL_actuator" },
              { "id": "Wheel_RR_actuator" }
            ]
          }
        },
        "actuators": [
          {
            "name": "Wheel_FL_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Front_Left",
            "origin": { "xyz": "0.6 -0.0000005 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          },
          {
            "name": "Wheel_FR_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Front_Right",
            "origin": { "xyz": "0.6 0.0000005 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          },
          {
            "name": "Wheel_RL_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Rear_Left",
            "origin": { "xyz": "-0.6 -0.0000005 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          },
          {
            "name": "Wheel_RR_actuator",
            "type": "wheel",
            "enabled": true,
            "parent-link": "Frame",
            "child-link": "Rear_Right",
            "origin": { "xyz": "-0.6 0.0000005 -0.3", "rpy-deg": "0 0 0" },
            "wheel-settings": {
              "normal-vector": "0.0 -1.0 0.0",
              "wheel-type": 0.0,
              "coeff-of-friction": 1.0,
              "coeff-of-wheel-torque": 0.040164,
              "engine": true,
              "steering": false,
              "brake": false,
              "smoothing-tc": 0.0
            }
          }
        ],
        "sensors": []
      }
    }
  ],
  "clock": {
    "type": "steppable",
    "step-ns": 3000000,
    "real-time-update-rate": 3000000,
    "pause-on-start": false
  },
  "home-geo-point": {
    "latitude": 47.641468,
    "longitude": -122.140165,
    "altitude": 122.0
  },
  "segmentation": {
    "initialize-ids": true,
    "ignore-existing": false,
    "use-owner-name": true
  }
}
)";

#endif  // PHYSICS_TEST_TEST_DATA_PHYSICS_TEST_ROVER_CONFIG_HPP_
