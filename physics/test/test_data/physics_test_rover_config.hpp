// Copyright (C) Microsoft Corporation.
// Copyright (C) 2025 IAMAI CONSULTING CORP

// MIT License. All rights reserved.

#ifndef PHYSICS_TEST_TEST_DATA_PHYSICS_TEST_ROVER_CONFIG_HPP_
#define PHYSICS_TEST_TEST_DATA_PHYSICS_TEST_ROVER_CONFIG_HPP_

// Two 4-wheel rover scene configs, both modeled on
// Autopilot/cobra_flex/sim/config/robot_cobra_flex_sim.jsonc (this repo's
// real differential-drive rover, no steering wheels), used by
// gtest_fast_physics.cpp to test FastPhysicsModel::CalcNextKinematicsWithWheels()'s
// two branches:
//
// - physics_test_diffdrive_rover_config: all four wheels "steering": false,
//   exercising the new differential-drive yaw model.
// - physics_test_ackermann_rover_config: identical geometry, but
//   Wheel_FL_actuator has "steering": true, exercising the pre-existing
//   Ackermann steering-angle yaw model to confirm it is untouched by the
//   new branch.
//
// Wheel origins (X = +-0.6, Y = +-0.5) give
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

#endif  // PHYSICS_TEST_TEST_DATA_PHYSICS_TEST_ROVER_CONFIG_HPP_
