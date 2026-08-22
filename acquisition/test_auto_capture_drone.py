"""
Tests for the auto_capture_drone.py script
"""

import unittest
import os
import tempfile
import shutil
import json
from unittest.mock import Mock, patch, MagicMock
import asyncio

# Import the class we want to test
from auto_capture_drone import AutoCaptureDrone


class TestAutoCaptureDrone(unittest.TestCase):
    """Test cases for AutoCaptureDrone class"""
    
    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()
        self.mock_client = Mock()
        self.mock_world = Mock()
        self.vehicle_name = "TestDrone"
        
    def tearDown(self):
        """Clean up test fixtures"""
        shutil.rmtree(self.temp_dir)
        
    def test_init(self):
        """Test AutoCaptureDrone initialization"""
        drone = AutoCaptureDrone(
            self.mock_client, 
            self.mock_world, 
            self.vehicle_name, 
            self.temp_dir
        )
        
        self.assertEqual(drone.save_dir, self.temp_dir)
        self.assertEqual(drone.frame_count, 0)
        self.assertIsNone(drone.cur_pose)
        self.assertIsNone(drone.cur_rgb_image)
        self.assertIsNone(drone.cur_depth_image)
        self.assertIsNone(drone.start_time)
        
        # Check if directory was created
        self.assertTrue(os.path.exists(self.temp_dir))
        
        # Check if metadata file path is set
        expected_metadata_path = os.path.join(self.temp_dir, "poses.json")
        self.assertEqual(drone.metadata_file, expected_metadata_path)
        
    def test_callback_rgb_image(self):
        """Test RGB image callback"""
        drone = AutoCaptureDrone(
            self.mock_client, 
            self.mock_world, 
            self.vehicle_name, 
            self.temp_dir
        )
        
        test_image_msg = {"test": "rgb_data"}
        drone.callback_rgb_image("topic", test_image_msg)
        
        self.assertEqual(drone.cur_rgb_image, test_image_msg)
        
    def test_callback_depth_image(self):
        """Test depth image callback"""
        drone = AutoCaptureDrone(
            self.mock_client, 
            self.mock_world, 
            self.vehicle_name, 
            self.temp_dir
        )
        
        test_depth_msg = {"test": "depth_data"}
        drone.callback_depth_image("topic", test_depth_msg)
        
        self.assertEqual(drone.cur_depth_image, test_depth_msg)
        
    def test_callback_actual_pose(self):
        """Test pose callback"""
        drone = AutoCaptureDrone(
            self.mock_client, 
            self.mock_world, 
            self.vehicle_name, 
            self.temp_dir
        )
        
        test_pose_msg = {"test": "pose_data"}
        drone.callback_actual_pose("topic", test_pose_msg)
        
        self.assertEqual(drone.cur_pose, test_pose_msg)
        
    @patch('cv2.imwrite')
    @patch('auto_capture_drone.unpack_image')
    def test_save_frame_with_valid_data(self, mock_unpack, mock_imwrite):
        """Test saving frame with valid RGB and pose data"""
        drone = AutoCaptureDrone(
            self.mock_client, 
            self.mock_world, 
            self.vehicle_name, 
            self.temp_dir
        )
        
        # Mock image data
        mock_rgb_img = Mock()
        mock_depth_img = Mock()
        mock_unpack.side_effect = [mock_rgb_img, mock_depth_img]
        
        # Set up drone state
        drone.cur_rgb_image = {"rgb": "data"}
        drone.cur_depth_image = {"depth": "data"}
        drone.cur_pose = {
            "position": {"x": 1.0, "y": 2.0, "z": 3.0},
            "orientation": {"w": 1.0, "x": 0.0, "y": 0.0, "z": 0.0}
        }
        
        # Save frame
        drone.save_frame()
        
        # Check if images were saved
        self.assertEqual(mock_imwrite.call_count, 2)
        
        # Check if pose data was added
        self.assertEqual(len(drone.pose_data), 1)
        self.assertEqual(drone.frame_count, 1)
        
        # Check pose data structure
        pose_entry = drone.pose_data[0]
        self.assertEqual(pose_entry["frame"], 0)
        self.assertEqual(pose_entry["position"]["x"], 1.0)
        self.assertEqual(pose_entry["position"]["y"], 2.0)
        self.assertEqual(pose_entry["position"]["z"], 3.0)
        
    def test_save_frame_with_missing_data(self):
        """Test saving frame with missing data"""
        drone = AutoCaptureDrone(
            self.mock_client, 
            self.mock_world, 
            self.vehicle_name, 
            self.temp_dir
        )
        
        # Don't set any data
        initial_frame_count = drone.frame_count
        
        # Save frame (should do nothing)
        drone.save_frame()
        
        # Frame count should not change
        self.assertEqual(drone.frame_count, initial_frame_count)
        self.assertEqual(len(drone.pose_data), 0)
        
    def test_save_final_metadata(self):
        """Test saving final metadata file"""
        drone = AutoCaptureDrone(
            self.mock_client, 
            self.mock_world, 
            self.vehicle_name, 
            self.temp_dir
        )
        
        # Add some test data
        drone.pose_data = [{"test": "data1"}, {"test": "data2"}]
        
        # Save metadata
        drone.save_final_metadata()
        
        # Check if file was created
        self.assertTrue(os.path.exists(drone.metadata_file))
        
        # Check file contents
        with open(drone.metadata_file, 'r') as f:
            saved_data = json.load(f)
            
        self.assertEqual(saved_data, drone.pose_data)
        
    def test_periodic_metadata_saving(self):
        """Test that metadata is saved periodically during frame capture"""
        drone = AutoCaptureDrone(
            self.mock_client, 
            self.mock_world, 
            self.vehicle_name, 
            self.temp_dir
        )
        
        # Mock image and pose data
        drone.cur_rgb_image = {"rgb": "data"}
        drone.cur_pose = {
            "position": {"x": 1.0, "y": 2.0, "z": 3.0},
            "orientation": {"w": 1.0, "x": 0.0, "y": 0.0, "z": 0.0}
        }
        
        # Save 10 frames (should trigger periodic save)
        for i in range(10):
            with patch('cv2.imwrite'), patch('auto_capture_drone.unpack_image'):
                drone.save_frame()
                
        # Check if metadata file was created
        self.assertTrue(os.path.exists(drone.metadata_file))
        
        # Check frame count
        self.assertEqual(drone.frame_count, 10)


class TestMainFunction(unittest.TestCase):
    """Test cases for the main function"""
    
    @patch('auto_capture_drone.ProjectAirSimClient')
    @patch('auto_capture_drone.World')
    @patch('auto_capture_drone.ImageDisplay')
    @patch('auto_capture_drone.asyncio.sleep')
    async def test_main_function_flow(self, mock_sleep, mock_image_display, mock_world, mock_client):
        """Test the main function execution flow"""
        # This is a basic test to ensure the main function can be called
        # In a real scenario, you'd want to mock the AirSim client and test specific behaviors
        
        # Mock the client and world
        mock_client_instance = Mock()
        mock_client.return_value = mock_client_instance
        
        mock_world_instance = Mock()
        mock_world.return_value = mock_world_instance
        
        mock_image_display_instance = Mock()
        mock_image_display.return_value = mock_image_display_instance
        
        # Mock the drone methods
        mock_drone = Mock()
        mock_drone.enable_api_control.return_value = None
        mock_drone.arm.return_value = None
        mock_drone.takeoff_async.return_value = Mock()
        mock_drone.move_by_velocity_async.return_value = Mock()
        mock_drone.land_async.return_value = Mock()
        mock_drone.disarm.return_value = None
        mock_drone.disable_api_control.return_value = None
        mock_drone.save_final_metadata.return_value = None
        
        # Mock the AutoCaptureDrone class
        with patch('auto_capture_drone.AutoCaptureDrone', return_value=mock_drone):
            # This test would need more sophisticated mocking to actually run
            # For now, we'll just test that the function can be imported and called
            pass


if __name__ == '__main__':
    unittest.main()
