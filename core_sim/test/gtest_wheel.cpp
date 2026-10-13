// Copyright (C) Microsoft Corporation.
// Copyright (C) 2025 IAMAI CONSULTING CORP

// MIT License. All rights reserved.

#include <string>
#include <vector>

#include "core_sim/actor/robot.hpp"
#include "core_sim/actuators/wheel.hpp"
#include "core_sim/config_json.hpp"
#include "core_sim/logger.hpp"
#include "core_sim/service_manager.hpp"
#include "gtest/gtest.h"
#include "json.hpp"
#include "state_manager.hpp"
#include "topic_manager.hpp"

namespace microsoft {
namespace projectairsim {

// Scene is the Robot's friend (see gtest_actuator.cpp); it grants the tests
// access to Robot's private constructor and Load().
class Scene {
 public:
  static Robot MakeWheelRobot() {
    auto logger_callback = [](const std::string&, LogLevel,
                              const std::string&) {};
    Logger logger(logger_callback);
    Transform origin = {{0, 0, 0}, {1, 0, 0, 0}};
    return Robot("TestRobotID", origin, logger, TopicManager(logger), "",
                 ServiceManager(logger), StateManager(logger));
  }

  static void LoadWheelRobot(Robot& robot, ConfigJson config_json) {
    robot.Load(config_json);
  }
};

}  // namespace projectairsim
}  // namespace microsoft

namespace projectairsim = microsoft::projectairsim;
using json = nlohmann::json;

namespace {

constexpr TimeNano kStepNanos = 10 * 1000 * 1000;  // 10 ms
constexpr int kSteps = 100;

projectairsim::Robot MakeRobot() {
  return projectairsim::Scene::MakeWheelRobot();
}

// A null wheel_settings leaves the "wheel-settings" block out so the loader
// defaults apply.
json WheelRobotConfig(const json& wheel_settings) {
  json wheel = {{"name", "W"},
                {"type", "wheel"},
                {"enabled", true},
                {"parent-link", "Frame"},
                {"child-link", "WheelLink"},
                {"origin", {{"xyz", "0 0 0"}, {"rpy-deg", "0 0 0"}}}};
  if (!wheel_settings.is_null()) wheel["wheel-settings"] = wheel_settings;
  return {{"links", json::array({{{"name", "Frame"}}, {{"name", "WheelLink"}}})},
          {"actuators", json::array({wheel})}};
}

json Settings(bool engine, bool steering, bool brake) {
  return {{"normal-vector", "0.0 -1.0 0.0"},
          {"wheel-type", 0.0},
          {"coeff-of-friction", 0.1},
          {"coeff-of-wheel-torque", 0.04},
          {"engine", engine},
          {"steering", steering},
          {"brake", brake},
          {"smoothing-tc", 0.0}};
}

projectairsim::Wheel& LoadWheel(projectairsim::Robot& robot,
                                const json& wheel_settings) {
  json config = WheelRobotConfig(wheel_settings);
  projectairsim::Scene::LoadWheelRobot(robot, config);
  auto& actuators = robot.GetActuators();
  EXPECT_EQ(actuators.size(), 1u);
  return static_cast<projectairsim::Wheel&>(actuators.at(0).get());
}

void Drive(projectairsim::Wheel& wheel, const std::vector<float>& signals) {
  for (int i = 0; i < kSteps; ++i) {
    wheel.UpdateActuatorOutput(std::vector<float>(signals), kStepNanos);
  }
}

}  // namespace

TEST(Wheel, ConnectedFlagsLoadedFromWheelSettings) {
  for (int mask = 0; mask < 8; ++mask) {
    const bool engine = mask & 1, steering = mask & 2, brake = mask & 4;
    auto robot = MakeRobot();
    auto& wheel = LoadWheel(robot, Settings(engine, steering, brake));
    EXPECT_EQ(wheel.IsEngineConnected(), engine) << mask;
    EXPECT_EQ(wheel.IsSteeringConnected(), steering) << mask;
    EXPECT_EQ(wheel.IsBrakeConnected(), brake) << mask;
  }
}

TEST(Wheel, ConnectedFlagsDefaultTrueWithoutWheelSettings) {
  auto robot = MakeRobot();
  auto& wheel = LoadWheel(robot, json());
  EXPECT_TRUE(wheel.IsEngineConnected());
  EXPECT_TRUE(wheel.IsSteeringConnected());
  EXPECT_TRUE(wheel.IsBrakeConnected());
}

TEST(Wheel, ConnectedChannelsDriveTheWheel) {
  auto robot = MakeRobot();
  auto& wheel = LoadWheel(robot, Settings(true, true, true));
  Drive(wheel, {1.0f, 0.5f, 0.0f});
  EXPECT_GT(wheel.GetRotatingSpeed(), 0.0f);
  EXPECT_NE(wheel.GetSteering(), 0.0f);
}

TEST(Wheel, DisconnectedEngineZeroesEngineChannel) {
  auto robot = MakeRobot();
  auto& wheel = LoadWheel(robot, Settings(false, true, true));
  Drive(wheel, {1.0f, 0.5f, 0.0f});
  EXPECT_FLOAT_EQ(wheel.GetRotatingSpeed(), 0.0f);
  EXPECT_NE(wheel.GetSteering(), 0.0f);
}

TEST(Wheel, DisconnectedSteeringZeroesSteeringChannel) {
  auto robot = MakeRobot();
  auto& wheel = LoadWheel(robot, Settings(true, false, true));
  Drive(wheel, {1.0f, 0.5f, 0.0f});
  EXPECT_FLOAT_EQ(wheel.GetSteering(), 0.0f);
  EXPECT_GT(wheel.GetRotatingSpeed(), 0.0f);
}

TEST(Wheel, DisconnectedBrakeZeroesBrakeChannel) {
  // Spin both wheels up, then command full brake and no engine: only the
  // wheel whose brake is connected slows faster than friction alone.
  auto robot_braked = MakeRobot();
  auto& braked = LoadWheel(robot_braked, Settings(true, true, true));
  auto robot_free = MakeRobot();
  auto& free_wheel = LoadWheel(robot_free, Settings(true, true, false));

  Drive(braked, {1.0f, 0.0f, 0.0f});
  Drive(free_wheel, {1.0f, 0.0f, 0.0f});
  ASSERT_FLOAT_EQ(braked.GetRotatingSpeed(), free_wheel.GetRotatingSpeed());

  Drive(braked, {0.0f, 0.0f, 1.0f});
  Drive(free_wheel, {0.0f, 0.0f, 1.0f});
  EXPECT_LT(braked.GetRotatingSpeed(), free_wheel.GetRotatingSpeed());
}

TEST(Wheel, ShortControlSignalVectorReadsMissingChannelsAsZero) {
  auto robot = MakeRobot();
  auto& wheel = LoadWheel(robot, Settings(true, true, true));
  Drive(wheel, {1.0f});
  EXPECT_GT(wheel.GetRotatingSpeed(), 0.0f);
  EXPECT_FLOAT_EQ(wheel.GetSteering(), 0.0f);
}
