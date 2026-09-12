// Copyright (C) Microsoft Corporation.
// Copyright (C) 2025 IAMAI CONSULTING CORP
//
// MIT License. All rights reserved.
// Tests for Robot class

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>

#include "core_sim/actor/robot.hpp"
#include "core_sim/config_json.hpp"
#include "core_sim/error.hpp"
#include "core_sim/logger.hpp"
#include "core_sim/runtime_components.hpp"
#include "core_sim/service_manager.hpp"
#include "gtest/gtest.h"
#include "json.hpp"
#include "state_manager.hpp"
#include "topic_manager.hpp"

using json = nlohmann::json;

namespace microsoft {
namespace projectairsim {

class Scene {
 public:
  static Robot MakeRobot(const std::string& id) {
    return MakeRobot(id, [](const std::string& component, LogLevel level,
                            const std::string& message) {});
  }

  static Robot MakeRobot(const std::string& id,
                         Logger::LoggerCallback log_callback) {
    Transform origin = {{0, 0, 0}, {1, 0, 0, 0}};
    Logger logger(log_callback);
    return Robot(id, origin, logger, TopicManager(logger), "",
                 ServiceManager(logger), StateManager(logger));
  }

  static void LoadRobot(Robot& robot, ConfigJson config_json) {
    robot.Load(config_json);
  }
};

}  // namespace projectairsim
}  // namespace microsoft

namespace {

// Minimal IController stand-ins used only to exercise
// Robot::Impl::OnEndUpdate()'s controller-teardown handling (see #634):
// a real controller's EndUpdate() commonly joins a per-robot actuator-API
// thread it owns (e.g. MavLinkApi::EndUpdate() joining its PX4 connect
// thread), and that join must not be able to block the caller forever.

// Stands in for a controller whose actuator-API thread is stuck and will
// never unblock on its own -- the scenario observed with a half-connected
// MavLinkApi::ConnectThread that never completed a PX4 handshake.
class HangingController : public microsoft::projectairsim::IController {
 public:
  void BeginUpdate() override {}
  void Reset() override {}
  void SetKinematics(const microsoft::projectairsim::Kinematics*) override {}
  void Update() override {}
  std::vector<float> GetControlSignals(const std::string&) override {
    return {};
  }
  const GimbalState& GetGimbalSignal(const std::string&) override {
    static GimbalState state{};
    return state;
  }

  // Never notified: simulates a per-robot thread with no cancellation and
  // no timeout, which is exactly what made the original unconditional
  // std::thread::join() in OnEndUpdate() hang forever.
  void EndUpdate() override {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [] { return false; });
  }

 private:
  std::mutex mutex_;
  std::condition_variable cv_;
};

// A well-behaved controller, to confirm normal teardown is unaffected.
class QuickController : public microsoft::projectairsim::IController {
 public:
  void BeginUpdate() override {}
  void Reset() override {}
  void SetKinematics(const microsoft::projectairsim::Kinematics*) override {}
  void Update() override {}
  std::vector<float> GetControlSignals(const std::string&) override {
    return {};
  }
  const GimbalState& GetGimbalSignal(const std::string&) override {
    static GimbalState state{};
    return state;
  }

  void EndUpdate() override { end_update_called = true; }

  bool end_update_called = false;
};

}  // namespace

namespace projectairsim = microsoft::projectairsim;

TEST(Robot, Constructor) {
  EXPECT_FALSE(projectairsim::Scene::MakeRobot("abc").IsLoaded());
}

TEST(Robot, LoadRobot) {
  json json = R"({
      "links": [ { "name": "Frame" } ]
    })"_json;
  auto robot = projectairsim::Scene::MakeRobot("a");
  projectairsim::Scene::LoadRobot(robot, json);
}

TEST(Robot, IsLoaded) {
  json json = R"({
      "links": [ { "name": "Frame" } ]
    })"_json;
  auto robot = projectairsim::Scene::MakeRobot("a");
  projectairsim::Scene::LoadRobot(robot, json);
  EXPECT_TRUE(robot.IsLoaded());
}

TEST(Robot, GetID) {
  json json = R"({
      "links": [ { "name": "Frame" } ]
    })"_json;
  auto robot = projectairsim::Scene::MakeRobot("a");
  projectairsim::Scene::LoadRobot(robot, json);
  EXPECT_EQ(robot.GetID(), "a");
}

TEST(Robot, GetType) {
  json json = R"({
      "links": [ { "name": "Frame" } ]
    })"_json;
  auto robot = projectairsim::Scene::MakeRobot("a");
  projectairsim::Scene::LoadRobot(robot, json);
  EXPECT_EQ(robot.GetType(), projectairsim::ActorType::kRobot);
}

TEST(Robot, GetLinks) {
  json json = R"({
        "links": [
          {
            "name": "link1",
            "visual": {
              "geometry": {
                  "type": "unreal_mesh",
                  "name": "Link1"
              }
            }
          },
          {
            "name": "link2",
            "visual": {
                "geometry": {
                  "type": "unreal_mesh",
                  "name": "Link2"
              }
            }
          }
        ]
      })"_json;

  auto robot = projectairsim::Scene::MakeRobot("a");
  projectairsim::Scene::LoadRobot(robot, json);
  EXPECT_EQ(robot.GetLinks().size(), 2);
}

// Regression test for #634: a controller whose EndUpdate() never returns
// (standing in for a stuck actuator-API thread, e.g. MavLinkApi's PX4
// connect thread half-connected but never completing its handshake) must
// not be able to block Robot::EndUpdate() forever -- and previously did,
// via an unconditional std::thread::join() in Robot::Impl::OnEndUpdate().
TEST(Robot, EndUpdateDoesNotHangOnStuckController) {
  json json = R"({
      "links": [ { "name": "Frame" } ]
    })"_json;

  std::mutex log_mutex;
  std::vector<std::string> error_messages;
  auto robot = projectairsim::Scene::MakeRobot(
      "a", [&](const std::string& component,
              microsoft::projectairsim::LogLevel level,
              const std::string& message) {
        if (level == microsoft::projectairsim::LogLevel::kError) {
          std::lock_guard<std::mutex> lock(log_mutex);
          error_messages.push_back(message);
        }
      });
  projectairsim::Scene::LoadRobot(robot, json);
  robot.SetController(std::make_unique<HangingController>());
  robot.BeginUpdate();

  auto start = std::chrono::steady_clock::now();
  robot.EndUpdate();
  auto elapsed = std::chrono::steady_clock::now() - start;

  // Bounded, not the tens-of-minutes-to-forever a live deadlock would take;
  // generous relative to the production timeout to stay robust under a
  // loaded CI machine.
  EXPECT_LT(elapsed, std::chrono::seconds(15));

  // A scene reload/stop must fail loud, not silently, when this happens --
  // the original bug report's other complaint ("no diagnostic pointing at
  // the cause").
  std::lock_guard<std::mutex> lock(log_mutex);
  EXPECT_FALSE(error_messages.empty());
}

// Confirms the bounded teardown path above doesn't change behavior for the
// common case of a controller that ends promptly.
TEST(Robot, EndUpdateRunsWellBehavedControllerNormally) {
  json json = R"({
      "links": [ { "name": "Frame" } ]
    })"_json;

  auto robot = projectairsim::Scene::MakeRobot("a");
  projectairsim::Scene::LoadRobot(robot, json);
  auto controller = std::make_unique<QuickController>();
  QuickController* controller_ptr = controller.get();
  robot.SetController(std::move(controller));
  robot.BeginUpdate();

  robot.EndUpdate();

  EXPECT_TRUE(controller_ptr->end_update_called);
}
