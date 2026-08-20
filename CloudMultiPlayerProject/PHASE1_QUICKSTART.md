# Phase 1: Quick Start Guide - Running Project AirSim on Mac

This guide will help you get Project AirSim's Blocks environment running on your Mac with a basic Python client.

## Prerequisites Check

Before starting, verify you have:

```bash
# Check Xcode Command Line Tools
xcode-select -p

# Check CMake
cmake --version  # Should be 3.15+

# Check Python
python3 --version  # Should be 3.9+

# Check if Unreal Engine 5.6 is available
# Location should be similar to:
# /Users/Shared/Epic Games/UE_5.6/ or custom location
```

## Step-by-Step Setup

### 1. Build Project AirSim Libraries

```bash
cd /Users/victorbjorkgren/projects/DroneProject/ProjectAirSim

# Run the macOS build script
./build_macos.sh

# This will:
# - Build all C++ libraries (core_sim, physics, rendering, etc.)
# - Build mavlinkcom
# - Create the Plugin binaries
```

Expected output:
- Libraries in `build/lib/`
- Plugin ready for Unreal

**Troubleshooting**:
- If build fails, check `build/CMakeCache.txt` for configuration issues
- Ensure you have sufficient disk space (10+ GB)

### 2. Generate Unreal Project Files

```bash
cd unreal/Blocks

# Generate Xcode project files
./blocks_genprojfiles_vscode.sh

# Or if you prefer Xcode IDE:
# Right-click Blocks.uproject -> Generate Xcode Project
```

### 3. Open and Build in Unreal Editor

Option A - Xcode:
```bash
# Open the generated workspace
open "Blocks (Mac).xcworkspace"

# Build the project (Cmd+B)
# Run in editor (Cmd+R)
```

Option B - Unreal Editor (Recommended):
```bash
# If you have UE5.6 installed, just open the project
# It will automatically build
open Blocks.uproject
```

**First Launch Tips**:
- Initial shader compilation takes 5-15 minutes
- You should see the Blocks environment (cubes/buildings)
- Press Play (or Alt+P) to start simulation

### 4. Install Python Client

```bash
cd client/python

# Create virtual environment (recommended)
python3 -m venv venv
source venv/bin/activate

# Install the client library
pip install -e .

# Verify installation
python -c "import projectairsim; print('Success!')"
```

### 5. Run Your First Script

Create a test script `test_single_drone.py`:

```python
import asyncio
from projectairsim import ProjectAirSimClient
from projectairsim.helpers.world import World
from projectairsim.helpers.robots.drone import Drone
from projectairsim.utils.logging import projectairsim_log

async def main():
    # Connect to simulation server
    server = "127.0.0.1"
    client = ProjectAirSimClient(server)
    
    try:
        client.connect()
        projectairsim_log().info("Connected to Project AirSim!")
        
        # Load default scene
        world = World(client, delay_after_load_sec=2)
        
        # Create drone object
        drone = Drone(client, world, "Drone1")
        
        # Enable API control
        drone.enable_api_control()
        drone.arm()
        
        # Take off
        projectairsim_log().info("Taking off...")
        await drone.takeoff_async()
        await asyncio.sleep(2)
        
        # Move forward
        projectairsim_log().info("Moving forward...")
        await drone.move_by_velocity_async(vx=2, vy=0, vz=0, duration_sec=3)
        await asyncio.sleep(3)
        
        # Hover
        projectairsim_log().info("Hovering...")
        await drone.move_by_velocity_async(vx=0, vy=0, vz=0, duration_sec=2)
        await asyncio.sleep(2)
        
        # Get camera image
        projectairsim_log().info("Capturing image...")
        image = drone.get_camera_image("front_center", "scene")
        if image is not None:
            projectairsim_log().info(f"Got image: {image.shape}")
        
        # Land
        projectairsim_log().info("Landing...")
        await drone.land_async()
        await asyncio.sleep(3)
        
        projectairsim_log().info("Mission complete!")
        
    except Exception as e:
        projectairsim_log().error(f"Error: {e}", exc_info=True)
    finally:
        client.disconnect()

if __name__ == "__main__":
    asyncio.run(main())
```

### 6. Run the Test

Terminal 1 - Start Simulation:
```bash
cd unreal/Blocks

# If you built the packaged version:
# For macOS, the exact path depends on your build
# Usually in: Blocks/Binaries/Mac/Blocks.app

# OR just run from Unreal Editor (Press Play)
```

Terminal 2 - Run Python Script:
```bash
source venv/bin/activate
python test_single_drone.py
```

**Expected Output**:
```
[INFO] Connected to Project AirSim!
[INFO] Taking off...
[INFO] Moving forward...
[INFO] Hovering...
[INFO] Capturing image...
[INFO] Got image: (480, 640, 3)
[INFO] Landing...
[INFO] Mission complete!
```

## Common Issues & Solutions

### Issue: "Connection refused"
**Cause**: Simulation server not running

**Solution**:
- Make sure Unreal Editor is open with Blocks project
- Press "Play" to start simulation
- Check that port 41451 is not blocked

### Issue: "No drone named 'Drone1'"
**Cause**: Scene configuration doesn't include a drone

**Solution**:
- Check scene configuration in `~/Documents/ProjectAirSim/scene_default.jsonc`
- Or pass custom scene file to `World()` constructor:
  ```python
  world = World(client, "path/to/your/scene.jsonc")
  ```

### Issue: Python import errors
**Cause**: Client library not installed correctly

**Solution**:
```bash
cd client/python
pip uninstall projectairsim
pip install -e .
```

### Issue: Unreal Editor crashes on Play
**Cause**: Plugin not built correctly

**Solution**:
```bash
# Rebuild everything
./build_macos.sh
cd unreal/Blocks
# Regenerate project files
./blocks_genprojfiles_vscode.sh
# Open and build again
```

## Configuration Files

### Default Scene Location
```
~/Documents/ProjectAirSim/scene_default.jsonc
```

### Example Scene with Single Drone
```jsonc
{
  "clock": {
    "type": "steppable",
    "step-ns": 5000000
  },
  "actors": [
    {
      "type": "robot",
      "name": "Drone1",
      "origin": {
        "xyz": "0.0 0.0 -10.0",
        "rpy-deg": "0 0 0"
      },
      "ref": "robot_quadrotor_fastphysics.jsonc"
    }
  ]
}
```

### Robot Configuration Location
```
~/Documents/ProjectAirSim/robot_quadrotor_fastphysics.jsonc
```

## Verify Phase 1 Success

You've successfully completed Phase 1 if:

- ✅ Blocks environment opens in Unreal Editor
- ✅ You can press Play and see the environment
- ✅ Python client connects without errors
- ✅ Drone takes off and moves as commanded
- ✅ You can retrieve camera images
- ✅ Drone lands successfully

## Next Steps

Once Phase 1 is working:

1. **Experiment with controls**:
   - Try different velocities
   - Test rotation commands
   - Capture images from different angles

2. **Explore the API**:
   ```python
   # Get drone state
   state = drone.get_state()
   print(f"Position: {state.position}")
   print(f"Velocity: {state.velocity}")
   
   # Get sensor data
   imu = drone.get_imu_data()
   gps = drone.get_gps_data()
   ```

3. **Move to Phase 2**:
   - See `MULTIPLAYER_ROADMAP.md` Section: "Phase 2: Multiple Drones - Single Machine"

## Useful Commands Reference

```bash
# Build Project AirSim
./build_macos.sh

# Clean build
rm -rf build
./build_macos.sh

# Install Python client (dev mode)
cd client/python
pip install -e .

# Run example scripts
python client/python/example_user_scripts/hello_drone.py

# Check what's listening on ports
lsof -i :41451  # RPC port
lsof -i :41452  # Topics port
```

## Resources

- **Main Documentation**: `docs/`
- **API Reference**: `docs/api.md`
- **Config Guide**: `docs/config.md`
- **More Examples**: `client/python/example_user_scripts/`

## Getting Help

- **Discord**: https://discord.gg/XprQ2w64uj
- **GitHub Issues**: https://github.com/iamaisim/ProjectAirSim/issues
- **Docs**: Check `docs/faq.md`

---

**Ready to proceed?** Once this is working smoothly, you're ready for Phase 2! 🚁

---

*Part of the Multiplayer Kubernetes Deployment Project*  
*See `MULTIPLAYER_ROADMAP.md` for the complete journey*

