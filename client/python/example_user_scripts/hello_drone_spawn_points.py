"""
Copyright (C) Microsoft Corporation. 
Copyright (C) 2025 IAMAI CONSULTING CORP
MIT License.

Demonstrates flying a quadrotor drone with camera sensors.
This version checks for spawn points in the world and spawns the drone
at a random spawn point instead of using the coordinates from the scene config.
"""

import asyncio
import os
import random

from projectairsim import ProjectAirSimClient, Drone, World
from projectairsim.utils import projectairsim_log
from projectairsim.image_utils import ImageDisplay

# Async main function to wrap async drone commands
async def main():
    # Create a Project AirSim client
    host = os.getenv("PROJECTAIRSIM_HOST", "127.0.0.1")
    client = ProjectAirSimClient(address=host)

    # Initialize an ImageDisplay object to display camera sub-windows
    # Note: If you encounter OpenCV display errors, you can use headless mode:
    # image_display = ImageDisplay(headless=True)
    # Or let it auto-detect: image_display = ImageDisplay(headless=False)
    image_display = ImageDisplay(headless=False)

    try:
        # Connect to simulation environment
        client.connect()

        # Create a minimal scene config to load first (no actors - just to get spawn points)
        minimal_scene_config = {
            "id": "MinimalSceneForSpawnPoints",
            "actors": [],
            "clock": {
                "type": "steppable",
                "step-ns": 3000000,
                "real-time-update-rate": 3000000,
                "pause-on-start": False
            },
            "home-geo-point": {
                "latitude": 47.641468,
                "longitude": -122.140165,
                "altitude": 122.0
            },
            "segmentation": {
                "initialize-ids": True,
                "ignore-existing": False,
                "use-owner-name": True
            },
            "scene-type": "UnrealNative"
        }
        
        # Create World object and load minimal scene to get spawn points
        projectairsim_log().info("Loading minimal scene to discover spawn points...")
        world = World(client, delay_after_load_sec=0)
        world.load_scene(minimal_scene_config, delay_after_load_sec=1)

        # Get spawn points from the world
        spawn_points = world.get_spawn_points()
        
        # Create the full scene config with the drone actor
        if spawn_points:
            # Pick a random spawn point
            selected_spawn = random.choice(spawn_points)
            spawn_name = selected_spawn.get("display_name") or selected_spawn.get("name", "Unknown")
            spawn_translation = selected_spawn["translation"]
            
            projectairsim_log().info(
                f"Found {len(spawn_points)} spawn point(s). "
                f"Will spawn drone at random spawn point: {spawn_name} "
                f"at position ({spawn_translation['x']:.2f}, "
                f"{spawn_translation['y']:.2f}, {spawn_translation['z']:.2f})"
            )
            
            # Create scene config with drone at spawn point
            xyz_str = f"{spawn_translation['x']} {spawn_translation['y']} {spawn_translation['z']}"
        else:
            projectairsim_log().info(
                "No spawn points found in the world. Using default position."
            )
            # Use default position if no spawn points
            xyz_str = "-1.0 8.0 -4.0"

        # Create the full scene config with the drone actor
        scene_config_dict = {
            "id": "SceneBasicDrone",
            "actors": [
                {
                    "type": "robot",
                    "name": "Drone1",
                    "origin": {
                        "xyz": xyz_str,
                        "rpy-deg": "0 0 0"
                    },
                    "robot-config": "robot_quadrotor_fastphysics.jsonc"
                }
            ],
            "clock": minimal_scene_config["clock"],
            "home-geo-point": minimal_scene_config["home-geo-point"],
            "segmentation": minimal_scene_config["segmentation"],
            "scene-type": minimal_scene_config["scene-type"]
        }

        # Load the full scene with the drone
        projectairsim_log().info("Loading full scene with drone...")
        world.load_scene(scene_config_dict, delay_after_load_sec=2)

        # Create a Drone object to interact with a drone in the loaded sim world
        drone = Drone(client, world, "Drone1")

        # ------------------------------------------------------------------------------

        # Subscribe to chase camera sensor as a client-side pop-up window
        chase_cam_window = "ChaseCam"
        image_display.add_chase_cam(chase_cam_window)
        client.subscribe(
            drone.sensors["Chase"]["scene_camera"],
            lambda _, chase: image_display.receive(chase, chase_cam_window),
        )

        # Subscribe to the downward-facing camera sensor's RGB and Depth images
        rgb_name = "RGB-Image"
        image_display.add_image(rgb_name, subwin_idx=0)
        client.subscribe(
            drone.sensors["DownCamera"]["scene_camera"],
            lambda _, rgb: image_display.receive(rgb, rgb_name),
        )

        depth_name = "Depth-Image"
        image_display.add_image(depth_name, subwin_idx=2)
        client.subscribe(
            drone.sensors["DownCamera"]["depth_camera"],
            lambda _, depth: image_display.receive(depth, depth_name),
        )

        image_display.start()

        # ------------------------------------------------------------------------------

        # Set the drone to be ready to fly
        drone.enable_api_control()
        drone.arm()

        # ------------------------------------------------------------------------------

        projectairsim_log().info("takeoff_async: starting")
        takeoff_task = (
            await drone.takeoff_async()
        )  # schedule an async task to start the command

        # Example 1: Wait on the result of async operation using 'await' keyword
        await takeoff_task
        projectairsim_log().info("takeoff_async: completed")

        # ------------------------------------------------------------------------------

        # Command the drone to move up in NED coordinate system at 1 m/s for 4 seconds
        move_up_task = await drone.move_by_velocity_async(
            v_north=0.0, v_east=0.0, v_down=-1.0, duration=4.0
        )
        projectairsim_log().info("Move-Up invoked")

        await move_up_task
        projectairsim_log().info("Move-Up completed")

        # ------------------------------------------------------------------------------

        # Command the Drone to move down in NED coordinate system at 1 m/s for 4 seconds
        move_down_task = await drone.move_by_velocity_async(
            v_north=0.0, v_east=0.0, v_down=1.0, duration=4.0
        )  # schedule an async task to start the command
        projectairsim_log().info("Move-Down invoked")

        # Example 2: Wait for move_down_task to complete before continuing
        while not move_down_task.done():
            await asyncio.sleep(0.005)
        projectairsim_log().info("Move-Down completed")

        # ------------------------------------------------------------------------------

        projectairsim_log().info("land_async: starting")
        land_task = await drone.land_async()
        await land_task
        projectairsim_log().info("land_async: completed")

        # ------------------------------------------------------------------------------

        # Shut down the drone
        drone.disarm()
        drone.disable_api_control()

        # ------------------------------------------------------------------------------

    # logs exception on the console
    except Exception as err:
        projectairsim_log().error(f"Exception occurred: {err}", exc_info=True)

    finally:
        # Always disconnect from the simulation environment to allow next connection
        client.disconnect()

        image_display.stop()


if __name__ == "__main__":
    asyncio.run(main())  # Runner for async main function

