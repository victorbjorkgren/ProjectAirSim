#!/usr/bin/env python3
"""
Test script demonstrating how to use ImageDisplay in headless mode.
This script shows how to avoid OpenCV errors when running in environments
without display access (e.g., headless servers, SSH connections, etc.).
"""

import asyncio
import time
from projectairsim import Multirotor
from projectairsim.image_utils import ImageDisplay


async def main():
    """Main function demonstrating headless mode usage."""
    
    # Initialize the drone client
    drone = Multirotor()
    
    # Wait for connection
    print("Waiting for connection...")
    drone.wait_for_connection()
    print("Connected!")
    
    # Initialize ImageDisplay with headless mode
    # Option 1: Let it auto-detect headless environment
    image_display = ImageDisplay(headless=False)  # Will auto-detect
    
    # Option 2: Force headless mode
    # image_display = ImageDisplay(headless=True)
    
    # Option 3: Check current mode and change if needed
    # if image_display.is_headless():
    #     print("Running in headless mode - no GUI windows will be created")
    # else:
    #     print("Running with GUI windows enabled")
    
    # Add camera feeds
    rgb_name = "RGB"
    depth_name = "Depth"
    
    image_display.add_image(rgb_name, subwin_idx=0)
    image_display.add_image(depth_name, subwin_idx=1)
    
    # Subscribe to camera data
    drone.subscribe_to_camera(
        camera_name="0",
        image_type="Scene",
        callback=lambda _, rgb: image_display.receive(rgb, rgb_name),
    )
    
    drone.subscribe_to_camera(
        camera_name="0",
        image_type="DepthVis",
        callback=lambda _, depth: image_display.receive(depth, depth_name),
    )
    
    # Start the display (will run in headless mode if needed)
    image_display.start()
    
    # Take off and fly around a bit
    print("Taking off...")
    drone.takeoff_async().join()
    
    print("Flying around...")
    for i in range(5):
        # Move in a square pattern
        if i % 4 == 0:
            drone.move_to_position_async(0, 10, -10, 5).join()
        elif i % 4 == 1:
            drone.move_to_position_async(10, 10, -10, 5).join()
        elif i % 4 == 2:
            drone.move_to_position_async(10, 0, -10, 5).join()
        else:
            drone.move_to_position_async(0, 0, -10, 5).join()
        
        time.sleep(2)
    
    # Land
    print("Landing...")
    drone.land_async().join()
    
    # Stop the display
    image_display.stop()
    
    print("Test completed successfully!")


if __name__ == "__main__":
    # Run the async main function
    asyncio.run(main())

