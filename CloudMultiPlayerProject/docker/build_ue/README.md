# Building Unreal Engine Linux ARM64 from macOS

This directory contains tools to build Unreal Engine for **Linux ARM64** from macOS using Docker.

## Overview

Building Unreal Engine for Linux ARM64 is a complex process that:
- Takes **4+ hours** to complete
- Requires **~200GB** of disk space
- Produces an "installed build" that can be used for Project AirSim development
- **Runs natively on Apple Silicon** - no emulation needed! This avoids the .NET binary issues with x86-64 emulation.

## Prerequisites

1. **Docker Desktop** installed and running
2. **Unreal Engine Source Code** (you should have this at `~/projects/DroneProject/UE/`)
3. **Sufficient Disk Space**: At least 250GB free (200GB for build + 50GB buffer)

## Quick Start

1. **Ensure UE source is available**:
   ```bash
   ls ~/projects/DroneProject/UE/Setup.sh
   ```

2. **Run the build script**:
   ```bash
   cd CloudMultiPlayerProject/docker/build_ue
   ./build_ue_linux_from_mac.sh
   ```

3. **Wait for completion** (4+ hours):
   - Monitor progress: `docker logs -f $(docker ps -q --filter ancestor=ue-linux-builder:5.7)`
   - Check disk usage: `docker system df`

4. **After completion**, the built UE will be at:
   ```
   ~/projects/DroneProject/UE_Linux_Build/
   ```

## Custom Paths

You can customize the source and output paths:

```bash
export UE_SOURCE=/path/to/ue/source
export UE_OUTPUT=/path/to/output
./build_ue_linux_from_mac.sh
```

## Build Process

The build script:

1. **Builds Docker image** (`ue-linux-builder:5.7-arm64`) if needed
   - Contains all build dependencies (Ubuntu 22.04 ARM64, .NET ARM64, etc.)
   - Runs natively on Apple Silicon - no QEMU emulation!

2. **Runs the build** inside the container:
   - Builds directly from mounted UE source (no copy needed)
   - Runs `Setup.sh` (downloads dependencies into source tree)
   - Removes unnecessary platforms to save space
   - Runs `RunUAT.sh BuildGraph` to create installed build
   - Build artifacts (LocalBuilds/, Engine/Binaries/, etc.) are created in source tree
   - Copies final installed build to output directory

3. **Output location**: `~/projects/DroneProject/UE_Linux_Build/`

## Monitoring the Build

### View Logs
```bash
# Find container ID
docker ps

# Follow logs
docker logs -f <container-id>
```

### Check Disk Usage
```bash
# Docker disk usage
docker system df

# Host disk usage
df -h ~/projects/DroneProject/
```

### Check Build Progress
The build process has multiple stages:
1. Setup.sh (10-30 minutes)
2. Platform cleanup (1-2 minutes)
3. BuildGraph "Make Installed Build Linux" (3-4 hours)

You'll see progress messages in the logs.

## Troubleshooting

### "Docker daemon is not running"
- Start Docker Desktop
- Wait for it to fully start before running the script

### "Insufficient disk space"
- Free up at least 250GB
- Consider using an external drive:
  ```bash
  export UE_OUTPUT=/Volumes/ExternalDrive/UE_Linux_Build
  ```

### "Build failed" or "Setup.sh failed"
- Check that you have a valid UE source code
- Ensure you have internet connection (Setup.sh downloads dependencies)
- Check Docker logs for specific errors

### Build is taking too long
- Normal build time is 4+ hours
- First-time builds are slower
- If it's been 6+ hours, check logs for errors

### Container runs out of memory
- Increase Docker Desktop memory limit (Settings → Resources → Memory)
- Recommended: 8GB+ RAM allocated to Docker

## What Gets Built

The build creates an "installed build" which includes:
- Unreal Engine binaries for Linux
- Precompiled shaders (DDC)
- All necessary libraries and tools
- Ready to use for Project AirSim development

The installed build is self-contained and can be moved/copied to other locations.

## After the Build

Once the build completes:

1. **Set UE_ROOT**:
   ```bash
   export UE_ROOT=~/projects/DroneProject/UE_Linux_Build
   ```

2. **Verify the build**:
   ```bash
   ls $UE_ROOT/Engine/Build/BatchFiles/Linux/Build.sh
   ```

3. **Build Project AirSim Linux binary**:
   ```bash
   cd CloudMultiPlayerProject/docker/build
   ./build_linux_in_docker.sh
   ```

## Disk Space Management

After the build:
- **Source + Build Artifacts**: ~200GB (build artifacts are in source tree)
- **Installed Build Only**: ~40GB (copied to output directory)
- **For Project AirSim**: You only need the installed build from output directory

Build artifacts in the source tree (LocalBuilds/, Engine/Binaries/, Engine/Intermediate/) can be cleaned up if needed:
```bash
# Clean build artifacts (keeps source intact)
cd ~/projects/DroneProject/UE
rm -rf LocalBuilds Engine/Binaries Engine/Intermediate Engine/Saved
```

Note: Keeping build artifacts allows faster subsequent builds, but uses more disk space.

## Manual Build (Advanced)

If you prefer to run the build manually:

```bash
# Build the Docker image
docker build -t ue-linux-builder:5.7 -f Dockerfile .

# Run the build
docker run --rm \
  -v ~/projects/DroneProject/UE:/ue-source:ro \
  -v ~/projects/DroneProject/UE_Linux_Build:/ue-output \
  -e UE_SOURCE_DIR=/ue-source \
  -e UE_OUTPUT_DIR=/ue-output \
  ue-linux-builder:5.7
```

## Next Steps

After building UE Linux:
1. ✅ Build Project AirSim Linux binary (see `../build/README.md`)
2. ✅ Deploy with Docker Compose (see `../README.md`)
3. ✅ Continue with Phase 4 of the project

