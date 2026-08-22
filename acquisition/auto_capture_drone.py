"""
Copyright (C) Microsoft Corporation. 
Copyright (C) 2025 IAMAI CONSULTING CORP
MIT License.

Automatically moves a drone through a predefined flight path while capturing
images and poses at 3fps for data collection purposes.
"""

import asyncio
import os
import time
import json
from datetime import datetime
from abc import ABC, abstractmethod
from typing import List

from projectairsim import ProjectAirSimClient, Drone, World
from projectairsim.drone import YawControlMode
from projectairsim.utils import (
    projectairsim_log,
    unpack_image,
    quaternion_to_rpy
)
from projectairsim.image_utils import ImageDisplay


class FlightPathSegment:
    """Represents a single segment of a flight path"""
    
    def __init__(self, name: str, v_north: float, v_east: float, v_down: float, 
                 duration: float, description: str = ""):
        self.name = name
        self.v_north = v_north
        self.v_east = v_east
        self.v_down = v_down
        self.duration = duration
        self.description = description
    
    def __str__(self):
        return f"{self.name}: {self.description} (duration: {self.duration}s)"


class FlightPath(ABC):
    """Abstract base class for flight paths"""
    
    @abstractmethod
    def get_segments(self) -> List[FlightPathSegment]:
        """Return the list of flight path segments"""
        pass
    
    @abstractmethod
    def get_name(self) -> str:
        """Return the name of this flight path"""
        pass


class SimpleBoxFlightPath(FlightPath):
    """Simple box-shaped flight path (up, forward, right, down)"""
    
    def get_name(self) -> str:
        return "Simple Box"
    
    def get_segments(self) -> List[FlightPathSegment]:
        return [
            FlightPathSegment("Move-Up", 0.0, 0.0, -1.0, 4.0, "Move upward"),
            FlightPathSegment("Move-Forward", 1.0, 0.0, 0.0, 3.0, "Move forward"),
            FlightPathSegment("Move-Right", 0.0, 1.0, 0.0, 3.0, "Move right"),
            FlightPathSegment("Move-Down", 0.0, 0.0, 1.0, 4.0, "Move downward")
        ]


class YawRotationFlightPath(FlightPath):
    """Yaw rotation flight path - drone rotates 360 degrees in place"""
    
    def __init__(self, height: float = 4.0, yaw_rate: float = 1.0, 
                 rotation_duration: float = 3.0):
        self.height = height
        self.yaw_rate = yaw_rate  # radians per second
        self.rotation_duration = rotation_duration
    
    def get_name(self) -> str:
        return f"Yaw Rotation (h={self.height}m, {self.rotation_duration}s rotation)"
    
    def get_segments(self) -> List[FlightPathSegment]:
        segments: List[FlightPathSegment] = []
        
        # Takeoff and climb
        segments.append(FlightPathSegment("Climb", 0.0, 0.0, -1.0, self.height, "Climb to altitude"))
        
        # Yaw rotation in place (this will be handled specially in the flight manager)
        segments.append(FlightPathSegment(
            "Yaw-Rotation", 
            0.0, 0.0, 0.0,  # No translation movement
            self.rotation_duration,
            f"Rotate 360 degrees at {self.yaw_rate} rad/s"
        ))
        
        # Descent
        segments.append(FlightPathSegment("Descent", 0.0, 0.0, 1.0, self.height, "Return to ground"))
        
        return segments


class GridFlightPath(FlightPath):
    """Grid pattern flight path for systematic area coverage"""
    
    def __init__(self, grid_size: float = 10.0, grid_spacing: float = 2.0, 
                 flight_height: float = 8.0):
        self.grid_size = grid_size
        self.grid_spacing = grid_spacing
        self.flight_height = flight_height
    
    def get_name(self) -> str:
        return f"Grid ({self.grid_size}x{self.grid_size}m, spacing={self.grid_spacing}m)"
    
    def get_segments(self) -> List[FlightPathSegment]:
        segments: List[FlightPathSegment] = []
        
        # Climb to flight height
        segments.append(FlightPathSegment("Climb", 0.0, 0.0, -1.0, self.flight_height, "Climb to flight height"))
        
        # Calculate grid movements
        num_lines = int(self.grid_size / self.grid_spacing) + 1
        
        for i in range(num_lines):
            # Move to start of line
            if i > 0:
                segments.append(FlightPathSegment(
                    f"Position-{i}", 
                    0.0, -(self.grid_size/2), 0.0, 
                    2.0,
                    f"Fly line {i+1}"
                ))
            
            # Fly the line
            segments.append(FlightPathSegment(
                f"Line-{i+1}", 
                self.grid_size, 0.0, 0.0, 
                self.grid_size / 2,  # Assuming 2 m/s speed
                f"Fly line {i+1}"
            ))
        
        # Return to center and descend
        segments.append(FlightPathSegment("Return-Center", 0.0, 0.0, 0.0, 3.0, "Return to center"))
        segments.append(FlightPathSegment("Descent", 0.0, 0.0, 1.0, self.flight_height, "Return to ground"))
        
        return segments


class FlightPathManager:
    """Manages flight path execution and data capture"""
    
    def __init__(self, drone: 'AutoCaptureDrone', capture_fps: float = 3.0):
        self.drone = drone
        self.capture_fps = capture_fps
        self.capture_interval = 1.0 / capture_fps
        self.last_capture_time = time.time()  # Initialize to current time
    
    async def execute_flight_path(self, flight_path: FlightPath):
        """Execute a complete flight path with data capture"""
        segments = flight_path.get_segments()
        
        # Store current flight path for yaw rotation access
        self._current_flight_path = flight_path
        
        projectairsim_log().info(f"Executing flight path: {flight_path.get_name()}")
        projectairsim_log().info(f"Total segments: {len(segments)}")
        
        for i, segment in enumerate(segments):
            projectairsim_log().info(f"Segment {i+1}/{len(segments)}: {segment}")
            
            # Execute the segment
            await self._execute_segment(segment)
            
            # Brief pause between segments
            if i < len(segments) - 1:
                await asyncio.sleep(1.0)
    
    async def _execute_segment(self, segment: FlightPathSegment):
        """Execute a single flight path segment with data capture"""
        projectairsim_log().info(f"{segment.name}: starting")
        # self.drone.save_frame()
        # Check if this is a yaw rotation segment
        if segment.name == "Yaw-Rotation":
            # Get the yaw rate from the flight path
            flight_path = getattr(self, '_current_flight_path', None)
            if flight_path and hasattr(flight_path, 'yaw_rate'):
                yaw_rate = flight_path.yaw_rate
            else:
                yaw_rate = 1.0  # Default yaw rate
            
            # Execute yaw rotation - try move_by_velocity_async with yaw control
            projectairsim_log().info(f"Attempting yaw rotation with rate {yaw_rate} rad/s for {segment.duration}s")
            
            # Try move_by_velocity_async with yaw control instead
            # This should rotate the drone while hovering in place
            rotate_task = await self.drone.move_by_velocity_async(
                v_north=0.0,  # No translation
                v_east=0.0,   # No translation  
                v_down=0.0,   # No translation
                duration=segment.duration,
                yaw_control_mode=YawControlMode.MaxDegreeOfFreedom,  # Allow yaw control
                yaw_is_rate=True,    # yaw is a rate, not absolute
                yaw=yaw_rate         # yaw rate in rad/s
            )
            
            # Capture data during rotation
            rotation_start_time = time.time()
            capture_attempts = 0
            while not rotate_task.done():
                current_time = time.time()
                if current_time - self.last_capture_time >= self.capture_interval:
                    capture_attempts += 1
                    projectairsim_log().info(f"Yaw rotation capture attempt {capture_attempts} at t={current_time - rotation_start_time:.1f}s")
                    self.last_capture_time = current_time
                await asyncio.sleep(0.01)
            
            await rotate_task
        else:
            # Execute normal movement
            move_task = await self.drone.move_by_velocity_async(
                v_north=segment.v_north,
                v_east=segment.v_east,
                v_down=segment.v_down,
                duration=segment.duration
            )
            
            # Capture data during movement
            movement_start_time = time.time()
            capture_attempts = 0
            while not move_task.done():
                current_time = time.time()
                if current_time - self.last_capture_time >= self.capture_interval:
                    capture_attempts += 1
                    projectairsim_log().info(f"{segment.name} capture attempt {capture_attempts} at t={current_time - movement_start_time:.1f}s")
                    # self.drone.save_frame()
                    self.last_capture_time = current_time
                await asyncio.sleep(0.01)
            
            await move_task
        
        projectairsim_log().info(f"{segment.name}: completed")


class AutoCaptureDrone(Drone):
    """Extended Drone class with image and pose capture capabilities"""
    
    def __init__(self, client, world, vehicle_name, save_dir, capture_fps: float = 3.0):
        super().__init__(client, world, vehicle_name)
        self.save_dir = save_dir
        self.cur_pose = None
        self.cur_rgb_image = None
        self.cur_depth_image = None
        self.frame_count = 0
        self.start_time = None
        
        # Create save directory if it doesn't exist
        os.makedirs(save_dir, exist_ok=True)
        
        # Create metadata file for poses
        self.metadata_file = os.path.join(save_dir, "poses.json")
        self.pose_data = []

        self.last_capture_time = time.time()
        self.capture_interval = 1.0 / capture_fps
        
    def callback_rgb_image(self, topic, image_msg):
        """Callback for RGB image data"""
        self.cur_rgb_image = image_msg
        self.save_frame()
        
    def callback_depth_image(self, topic, depth_msg):
        """Callback for depth image data"""
        self.cur_depth_image = depth_msg
        
    def callback_actual_pose(self, topic, pose_msg):
        """Callback for actual pose data"""
        self.cur_pose = pose_msg
        
    def save_frame(self):
        """Save current frame with RGB, depth, and pose data"""
        current_time = time.time()
        if current_time - self.last_capture_time >= self.capture_interval:
            self.last_capture_time = current_time
        else:
            return
        
        if self.cur_rgb_image is None or self.cur_pose is None:
            projectairsim_log().warning(f"Frame {self.frame_count}: Missing data - RGB: {self.cur_rgb_image is not None}, Pose: {self.cur_pose is not None}")
            return
        
        # Debug: Log every save attempt
        projectairsim_log().info(f"Saving frame {self.frame_count}...")
            
        timestamp = time.time()
        frame_filename = f"frame_{self.frame_count:06d}"
        
        # Save RGB image
        if self.cur_rgb_image is not None:
            rgb_img = unpack_image(self.cur_rgb_image)
            rgb_path = os.path.join(self.save_dir, f"{frame_filename}_rgb.png")
            import cv2
            cv2.imwrite(rgb_path, rgb_img)
            
        # Save depth image (if available from DownCamera)
        depth_image_filename = None
        if self.cur_depth_image is not None:
            depth_img = unpack_image(self.cur_depth_image)
            depth_path = os.path.join(self.save_dir, f"{frame_filename}_depth.png")
            cv2.imwrite(depth_path, depth_img)
            depth_image_filename = f"{frame_filename}_depth.png"
            
        # Extract pose information
        pos = self.cur_pose["position"]
        rot = self.cur_pose["orientation"]
        rpy = quaternion_to_rpy(rot["w"], rot["x"], rot["y"], rot["z"])
        
        # Store pose data
        pose_entry = {
            "frame": self.frame_count,
            "timestamp": timestamp,
            "position": {
                "x": pos["x"],
                "y": pos["y"], 
                "z": pos["z"]
            },
            "orientation": {
                "w": rot["w"],
                "x": rot["x"],
                "y": rot["y"],
                "z": rot["z"]
            },
            "rpy": {
                "roll": rpy[0],
                "pitch": rpy[1],
                "yaw": rpy[2]
            },
            "rgb_image": f"{frame_filename}_rgb.png",
            "depth_image": depth_image_filename
        }
        
        self.pose_data.append(pose_entry)
        self.frame_count += 1
        
        # Debug: Log successful save
        projectairsim_log().info(f"Successfully saved frame {self.frame_count-1}")
        
        # Save metadata periodically
        if self.frame_count % 10 == 0:
            with open(self.metadata_file, 'w') as f:
                json.dump(self.pose_data, f, indent=2)
                
    def save_final_metadata(self):
        """Save final metadata file"""
        with open(self.metadata_file, 'w') as f:
            json.dump(self.pose_data, f, indent=2)
    
    def create_flight_gif(self, gif_filename="flight_sequence.gif", duration=200):
        """Create a GIF from all captured RGB images"""
        try:
            import cv2
            from PIL import Image
            import glob
            
            # Find all RGB images
            rgb_pattern = os.path.join(self.save_dir, "frame_*_rgb.png")
            rgb_files = sorted(glob.glob(rgb_pattern))
            
            if not rgb_files:
                projectairsim_log().warning("No RGB images found to create GIF")
                return
            
            projectairsim_log().info(f"Creating GIF from {len(rgb_files)} images...")
            
            # Load and resize images
            images = []
            for rgb_file in rgb_files:
                # Load with OpenCV
                img = cv2.imread(rgb_file)
                if img is not None:
                    # Convert BGR to RGB
                    img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
                    # Resize to reasonable size for GIF (optional)
                    height, width = img_rgb.shape[:2]
                    if width > 640:  # Resize if too large
                        scale = 640 / width
                        new_width = int(width * scale)
                        new_height = int(height * scale)
                        img_rgb = cv2.resize(img_rgb, (new_width, new_height))
                    
                    # Convert to PIL Image
                    pil_img = Image.fromarray(img_rgb)
                    images.append(pil_img)
            
            if images:
                # Create GIF
                gif_path = os.path.join(self.save_dir, gif_filename)
                images[0].save(
                    gif_path,
                    save_all=True,
                    append_images=images[1:],
                    duration=duration,  # milliseconds per frame
                    loop=0  # infinite loop
                )
                projectairsim_log().info(f"GIF created: {gif_path}")
                return gif_path
            else:
                projectairsim_log().warning("No valid images found for GIF creation")
                
        except ImportError as e:
            projectairsim_log().error(f"Missing required packages for GIF creation: {e}")
            projectairsim_log().info("Install PIL with: pip install Pillow")
        except Exception as e:
            projectairsim_log().error(f"Error creating GIF: {e}")


class AutoCaptureConfig:
    """Configuration class for auto capture settings"""
    
    def __init__(self, 
                 flight_path_type: str = "simple_box",
                 capture_fps: float = 3.0,
                 scene_file: str = "scene_fpv_drone.jsonc",
                 delay_after_load_sec: float = 2.0,
                 create_gif: bool = True,
                 gif_duration: int = 200,
                 **flight_path_params):
        self.flight_path_type = flight_path_type
        self.capture_fps = capture_fps
        self.scene_file = scene_file
        self.delay_after_load_sec = delay_after_load_sec
        self.create_gif = create_gif
        self.gif_duration = gif_duration
        self.flight_path_params = flight_path_params
    
    @classmethod
    def from_json(cls, config_file: str = "flight_path_config.json") -> 'AutoCaptureConfig':
        """Load configuration from a JSON file"""
        try:
            with open(config_file, 'r') as f:
                config_data = json.load(f)
            
            # Extract flight path parameters
            flight_path_type = config_data.get("flight_path_type", "simple_box")
            flight_path_params = config_data.get("flight_paths", {}).get(flight_path_type, {}).get("parameters", {})
            
            return cls(
                flight_path_type=flight_path_type,
                capture_fps=config_data.get("capture_fps", 3.0),
                scene_file=config_data.get("scene_file", "scene_fpv_drone.jsonc"),
                delay_after_load_sec=config_data.get("delay_after_load_sec", 2.0),
                create_gif=config_data.get("create_gif", True),
                gif_duration=config_data.get("gif_duration", 200),
                **flight_path_params
            )
        except FileNotFoundError:
            projectairsim_log().warning(f"Config file {config_file} not found, using defaults")
            return cls()
        except json.JSONDecodeError as e:
            projectairsim_log().error(f"Error parsing config file: {e}")
            return cls()
    
    def create_flight_path(self) -> FlightPath:
        """Create a flight path based on configuration"""
        if self.flight_path_type == "simple_box":
            return SimpleBoxFlightPath()
        elif self.flight_path_type == "yaw_rotation":
            return YawRotationFlightPath(**self.flight_path_params)
        elif self.flight_path_type == "grid":
            return GridFlightPath(**self.flight_path_params)
        else:
            raise ValueError(f"Unknown flight path type: {self.flight_path_type}")


async def main():
    """Main function for automatic drone movement and data capture"""
    
    # Configuration - load from JSON file or use defaults
    # You can easily change flight paths by editing flight_path_config.json
    config = AutoCaptureConfig.from_json("flight_path_config.json")
    
    # Alternatively, you can still configure programmatically:
    # config = AutoCaptureConfig(
    #     flight_path_type="spiral",  # Options: "simple_box", "spiral", "grid"
    #     capture_fps=5.0,
    #     radius=8.0, height=15.0, spiral_turns=5, duration_per_turn=10.0
    # )
    
    # Create save directory
    save_dir = f"./captured_data_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
    
    # Create a Project AirSim client
    client = ProjectAirSimClient()
    
    # Initialize an ImageDisplay object (headless for data collection)
    image_display = ImageDisplay(headless=True)
    
    try:
        # Connect to simulation environment
        client.connect()
        
        # Create a World object to interact with the sim world and load a scene
        world = World(client, config.scene_file, delay_after_load_sec=config.delay_after_load_sec)
        
        # Create an AutoCaptureDrone object
        drone = AutoCaptureDrone(client, world, "Drone1", save_dir)
        
        # Create flight path manager
        flight_manager = FlightPathManager(drone, config.capture_fps)
        
        # ------------------------------------------------------------------------------
        
        # Subscribe to camera sensors (using FPV camera mounted on the drone)
        if "scene_camera" in drone.sensors["FPVCamera"]:
            client.subscribe(
                drone.sensors["FPVCamera"]["scene_camera"],
                drone.callback_rgb_image
            )
        
        # Subscribe to depth camera from the FPV camera
        if "depth_camera" in drone.sensors["FPVCamera"]:
            client.subscribe(
                drone.sensors["FPVCamera"]["depth_camera"],
                drone.callback_depth_image
            )
        
        client.subscribe(
            drone.robot_info["actual_pose"],
            drone.callback_actual_pose
        )
        
        # Start image display (even in headless mode for processing)
        image_display.start()
        
        # ------------------------------------------------------------------------------
        
        # Set the drone to be ready to fly
        drone.enable_api_control()
        drone.arm()
        
        # ------------------------------------------------------------------------------
        
        projectairsim_log().info("Starting automatic flight path with data capture")
        projectairsim_log().info(f"Data will be saved to: {save_dir}")
        projectairsim_log().info(f"Capture rate: {config.capture_fps} fps")
        
        # Start capture timer
        drone.start_time = time.time()
        
        # ------------------------------------------------------------------------------
        
        # Takeoff
        projectairsim_log().info("takeoff_async: starting")
        takeoff_task = await drone.takeoff_async()
        await takeoff_task
        projectairsim_log().info("takeoff_async: completed")
        
        # Wait a moment for stabilization
        await asyncio.sleep(2.0)
        
        # ------------------------------------------------------------------------------
        
        # Execute the configured flight path
        flight_path = config.create_flight_path()
        await flight_manager.execute_flight_path(flight_path)
        
        # ------------------------------------------------------------------------------
        
        # Land
        projectairsim_log().info("land_async: starting")
        land_task = await drone.land_async()
        await land_task
        projectairsim_log().info("land_async: completed")
        
        # ------------------------------------------------------------------------------
        
        # Save final metadata
        drone.save_final_metadata()
        
        # Create GIF from captured images (if enabled)
        gif_path = None
        if config.create_gif:
            gif_path = drone.create_flight_gif(duration=config.gif_duration)
        
        # Shut down the drone
        drone.disarm()
        drone.disable_api_control()
        
        # ------------------------------------------------------------------------------
        
        projectairsim_log().info(f"Data collection completed!")
        projectairsim_log().info(f"Total frames captured: {drone.frame_count}")
        projectairsim_log().info(f"Data saved to: {save_dir}")
        if gif_path:
            projectairsim_log().info(f"Flight GIF created: {gif_path}")
        
    except Exception as err:
        projectairsim_log().error(f"Exception occurred: {err}", exc_info=True)
        
    finally:
        # Always disconnect from the simulation environment
        client.disconnect()
        image_display.stop()


if __name__ == "__main__":
    asyncio.run(main())
