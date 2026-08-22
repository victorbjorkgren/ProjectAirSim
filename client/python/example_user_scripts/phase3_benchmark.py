import asyncio
import argparse
import time
import sys
import psutil
import os
from projectairsim import ProjectAirSimClient, Drone, World
from projectairsim.types import ImageType

async def measure_performance(duration=10, get_images=True):
    client = ProjectAirSimClient()
    
    # Retry connection logic
    connected = False
    for i in range(5):
        try:
            print(f"Attempting connection {i+1}/5...")
            client.connect()
            connected = True
            break
        except Exception as e:
            print(f"Connection failed: {e}")
            time.sleep(2)
            
    if not connected:
        print("Failed to connect after multiple attempts")
        sys.exit(1)

    # Load scene
    print("Loading scene...")
    try:
        # World init does get_topic_info which might timeout if sim is busy loading
        # We reduce timeout to fail faster for testing
        world = World(client, "scene_basic_drone.jsonc", delay_after_load_sec=2)
        drone = Drone(client, world, "Drone1")
    except Exception as e:
        print(f"Failed to init world/drone: {e}")
        sys.exit(1)

    drone.enable_api_control()
    drone.arm()

    print("Takeoff...")
    await drone.takeoff_async()
    
    start_time = time.time()
    frame_count = 0
    success_image_count = 0
    failed_image_count = 0
    
    print(f"Starting benchmark loop for {duration} seconds...")

    while time.time() - start_time < duration:
        # Move slightly to ensure activity
        await drone.move_by_velocity_async(0.5, 0, 0, duration=0.1)
        
        if get_images:
            try:
                # Using DownCamera as it is standard in basic scene
                images = drone.get_images("DownCamera", [ImageType.SCENE, ImageType.DEPTH_VIS])
                if len(images) >= 1:
                    success_image_count += 1
                else:
                    failed_image_count += 1
            except Exception as e:
                # print(f"Image capture error: {e}")
                failed_image_count += 1
        
        frame_count += 1
        await asyncio.sleep(0.01)

    end_time = time.time()
    total_time = end_time - start_time
    avg_fps = frame_count / total_time
    
    print("-" * 40)
    print(f"Benchmark Results (Image Capture: {get_images})")
    print(f"Duration: {total_time:.2f}s")
    print(f"Loops/FPS (Client Side): {avg_fps:.2f}")
    if get_images:
        print(f"Images Captured Sets: {success_image_count}")
        print(f"Images Failed Sets: {failed_image_count}")
    print("-" * 40)

    print("Landing...")
    await drone.land_async()
    drone.disarm()
    drone.disable_api_control()
    client.disconnect()

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--no-images", action="store_true", help="Disable image capture")
    args = parser.parse_args()

    try:
        # Use a timeout for the whole run to avoid hanging
        asyncio.run(asyncio.wait_for(measure_performance(duration=15, get_images=not args.no_images), timeout=60))
    except asyncio.TimeoutError:
        print("Benchmark timed out!")
        sys.exit(1)
    except KeyboardInterrupt:
        pass
    except Exception as e:
        print(f"Run failed: {e}")
        sys.exit(1)
