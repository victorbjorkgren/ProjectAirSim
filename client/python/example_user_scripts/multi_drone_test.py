"""
Copyright (C) 2025 IAMAI CONSULTING CORP
MIT License.

Demonstrates controlling three drones flying simultaneously.
"""

import asyncio

from projectairsim import Drone, ProjectAirSimClient, World
from projectairsim.utils import projectairsim_log
from projectairsim.image_utils import ImageDisplay


# Async function to wrap async drone commands
async def fly_three_drones(drone1: Drone, drone2: Drone, drone3: Drone, world: World):
    """Move three drones simultaneously using Async calls along a box pattern

    Arguments:
        drone1 {Drone} -- ProjectAirSim Drone 1
        drone2 {Drone} -- ProjectAirSim Drone 2
        drone3 {Drone} -- ProjectAirSim Drone 3
    """

    # ------------------------------------------------------------------------------

    # Set the drones to be ready to fly
    projectairsim_log().info("Arming drones...")
    drone1.enable_api_control()
    drone1.arm()
    drone2.enable_api_control()
    drone2.arm()
    drone3.enable_api_control()
    drone3.arm()

    # ------------------------------------------------------------------------------

    cmd_duration_sim_sec = 3  # seconds sim-time
    velocity_mps = 1

    projectairsim_log().info("Move Up")
    await asyncio.gather(
        await drone1.move_by_velocity_async(0, 0, -velocity_mps, cmd_duration_sim_sec),
        await drone2.move_by_velocity_async(0, 0, -velocity_mps, cmd_duration_sim_sec),
        await drone3.move_by_velocity_async(0, 0, -velocity_mps, cmd_duration_sim_sec),
    )

    projectairsim_log().info("Formation Flight: V-Formation")
    # Drone 1 (Center) moves forward
    # Drone 2 (Right) moves forward and right
    # Drone 3 (Left) moves forward and left
    await asyncio.gather(
        await drone1.move_by_velocity_async(velocity_mps, 0, 0, cmd_duration_sim_sec),
        await drone2.move_by_velocity_async(velocity_mps, 0.5, 0, cmd_duration_sim_sec),
        await drone3.move_by_velocity_async(velocity_mps, -0.5, 0, cmd_duration_sim_sec),
    )

    projectairsim_log().info("Independent Waypoints")
    # Drone 1 hovers
    # Drone 2 moves Right
    # Drone 3 moves Left
    await asyncio.gather(
        await drone1.move_by_velocity_async(0, 0, 0, cmd_duration_sim_sec), # Hover for duration
        await drone2.move_by_velocity_async(0, velocity_mps, 0, cmd_duration_sim_sec),
        await drone3.move_by_velocity_async(0, -velocity_mps, 0, cmd_duration_sim_sec),
    )
    # Note: hover_async might not be awaitable like move_by_velocity_async in the same way depending on impl, 
    # but usually it is. If it is instant, we might need a sleep. 
    # Let's use move_by_velocity(0,0,0) for hover to be safe and consistent with duration.
    
    projectairsim_log().info("Re-grouping")
    await asyncio.gather(
        await drone1.move_by_velocity_async(0, 0, 0, cmd_duration_sim_sec),
        await drone2.move_by_velocity_async(0, -velocity_mps, 0, cmd_duration_sim_sec),
        await drone3.move_by_velocity_async(0, velocity_mps, 0, cmd_duration_sim_sec),
    )

    projectairsim_log().info("Move Down")
    await asyncio.gather(
        await drone1.move_by_velocity_async(0, 0, velocity_mps, cmd_duration_sim_sec),
        await drone2.move_by_velocity_async(0, 0, velocity_mps, cmd_duration_sim_sec),
        await drone3.move_by_velocity_async(0, 0, velocity_mps, cmd_duration_sim_sec),
    )

    # ------------------------------------------------------------------------------

    # Shut down the drones
    projectairsim_log().info("Disarming drones...")
    drone1.disarm()
    drone1.disable_api_control()
    drone2.disarm()
    drone2.disable_api_control()
    drone3.disarm()
    drone3.disable_api_control()


if __name__ == "__main__":
    # Create a Project AirSim client
    client = ProjectAirSimClient()

    # Initialize an ImageDisplay object to display camera sub-windows
    image_display = ImageDisplay()

    try:
        # Connect to simulation environment
        client.connect()

        # Create a World object to interact with the sim world and load a scene
        world = World(client, "multi_drone_scene.jsonc", delay_after_load_sec=2)

        # Create Drone objects to interact with the drones in the loaded sim world
        drone1 = Drone(client, world, "Drone1")
        drone2 = Drone(client, world, "Drone2")
        drone3 = Drone(client, world, "Drone3")

        # Subscribe to Drone1's chase camera sensor
        chase_cam_window = "ChaseCam"
        image_display.add_chase_cam(chase_cam_window)
        
        # Check if sensors are available before subscribing (basic check)
        if "Chase" in drone1.sensors:
             client.subscribe(
                drone1.sensors["Chase"]["scene_camera"],
                lambda _, chase: image_display.receive(chase, chase_cam_window),
            )

        image_display.start()

        # Run the async function to execute the async drone commands
        asyncio.run(fly_three_drones(drone1, drone2, drone3, world))

    except Exception as err:
        projectairsim_log().error(f"Exception occurred: {err}", exc_info=True)

    finally:
        # Always disconnect from the simulation environment to allow next connection
        client.disconnect()

        image_display.stop()

