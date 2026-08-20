#!/bin/bash
# Wrapper script to build Unreal Engine Linux from macOS using Docker
# This script handles Docker setup and runs the build

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UE_SOURCE="${UE_SOURCE:-$HOME/projects/DroneProject/UE}"
# Output directory is optional - if not set, we'll use the source location directly
UE_OUTPUT="${UE_OUTPUT:-}"

echo "======================================================================="
echo "Unreal Engine Linux Build from macOS"
echo "======================================================================="
echo "UE Source: $UE_SOURCE"
if [ -n "$UE_OUTPUT" ]; then
    echo "Output: $UE_OUTPUT (optional copy location)"
else
    echo "Output: Will use source location directly ($UE_SOURCE/LocalBuilds/Engine/Linux)"
fi
echo ""

# Check Docker
if ! command -v docker &> /dev/null; then
    echo "ERROR: Docker is not installed. Please install Docker Desktop for Mac."
    exit 1
fi

if ! docker info &> /dev/null; then
    echo "ERROR: Docker daemon is not running. Please start Docker Desktop."
    exit 1
fi

echo "✓ Docker is installed and running"
echo ""

# Check UE source
if [ ! -d "$UE_SOURCE" ]; then
    echo "ERROR: UE source directory not found: $UE_SOURCE"
    echo ""
    echo "Set UE_SOURCE environment variable or ensure UE is at:"
    echo "  $HOME/projects/DroneProject/UE"
    exit 1
fi

if [ ! -f "$UE_SOURCE/Setup.sh" ]; then
    echo "ERROR: Setup.sh not found in $UE_SOURCE"
    echo "This doesn't appear to be a valid UE source directory"
    exit 1
fi

echo "✓ UE source found: $UE_SOURCE"
echo ""

# Create output directory if specified
if [ -n "$UE_OUTPUT" ]; then
    mkdir -p "$UE_OUTPUT"
    echo "✓ Output directory: $UE_OUTPUT"
else
    echo "✓ Will use build location directly in source tree"
fi
echo ""

# Check disk space (warn if less than 250GB free)
# Check space on the source directory (where build artifacts will be created)
CHECK_DIR="${UE_OUTPUT:-$UE_SOURCE}"
AVAILABLE_SPACE=$(df -BG "$CHECK_DIR" | tail -1 | awk '{print $4}' | sed 's/G//')
if [ "$AVAILABLE_SPACE" -lt 250 ]; then
    echo "⚠ WARNING: Less than 250GB free space available"
    echo "  Available: ${AVAILABLE_SPACE}GB"
    echo "  Recommended: 250GB+ for a complete build"
    echo "  Checking: $CHECK_DIR"
    echo ""
    read -p "Continue anyway? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Build Docker image if it doesn't exist
IMAGE_NAME="ue-linux-builder:5.7-arm64"
if ! docker image inspect "$IMAGE_NAME" >/dev/null 2>&1; then
    echo "======================================================================="
    echo "Building Docker image: $IMAGE_NAME"
    echo "======================================================================="
    echo "This may take a few minutes..."
    echo "Note: Building for linux/arm64 platform (native on Apple Silicon, no emulation!)"
    docker build --platform linux/arm64 -t "$IMAGE_NAME" -f "$SCRIPT_DIR/Dockerfile" "$SCRIPT_DIR"
    echo ""
fi

echo "======================================================================="
echo "Starting UE Linux Build"
echo "======================================================================="
echo ""
echo "IMPORTANT:"
echo "  - This build will take 4+ hours"
echo "  - It requires ~200GB disk space"
echo "  - The build runs in the container"
echo ""
echo "Building for: ARM64 Linux (native on Apple Silicon - no emulation!)"
echo ""
echo "To monitor progress in another terminal:"
echo "  docker logs -f \$(docker ps -q --filter ancestor=$IMAGE_NAME)"
echo ""
echo "To check disk usage:"
echo "  docker system df"
echo ""
read -p "Ready to start? (y/N) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Build cancelled."
    exit 1
fi

echo ""
echo "Starting build at: $(date)"
echo ""

# Clean up any existing container with the same name
if docker ps --format '{{.Names}}' | grep -q "^ue-linux-builder$"; then
    echo "⚠ WARNING: Container 'ue-linux-builder' is currently running."
    echo "This might be from a previous build."
    echo ""
    read -p "Stop and remove it? (y/N) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Stopping and removing existing container..."
        docker rm -f ue-linux-builder
    else
        echo "Cannot proceed with existing container running."
        echo "Either wait for it to finish or stop it manually:"
        echo "  docker rm -f ue-linux-builder"
        exit 1
    fi
elif docker ps -a --format '{{.Names}}' | grep -q "^ue-linux-builder$"; then
    echo "Removing stopped container 'ue-linux-builder'..."
    docker rm ue-linux-builder >/dev/null 2>&1 || true
fi

# Run the build
# Mount source as read-write so build can create artifacts (LocalBuilds/, Engine/Binaries/, etc.)
# The source files themselves won't be modified, only build outputs will be created
# Using linux/arm64 platform for native execution on Apple Silicon (no emulation!)
DOCKER_ARGS=(
    --rm
    --platform linux/arm64
    --name ue-linux-builder
    -v "$UE_SOURCE:/ue-source"
    -e UE_SOURCE_DIR=/ue-source
)

# Only mount output directory if specified
if [ -n "$UE_OUTPUT" ]; then
    DOCKER_ARGS+=(-v "$UE_OUTPUT:/ue-output")
    DOCKER_ARGS+=(-e UE_OUTPUT_DIR=/ue-output)
fi

# Run the build with better error handling
if ! docker run "${DOCKER_ARGS[@]}" "$IMAGE_NAME"; then
    BUILD_EXIT_CODE=$?
    echo ""
    echo "======================================================================="
    echo "Build failed or was interrupted (exit code: $BUILD_EXIT_CODE)"
    echo "======================================================================="
    echo ""
    echo "Possible causes:"
    echo "  - Docker daemon stopped/crashed"
    echo "  - Out of memory"
    echo "  - Out of disk space"
    echo "  - Build process error"
    echo ""
    echo "To resume:"
    echo "  1. Ensure Docker Desktop is running"
    echo "  2. Check disk space: df -h"
    echo "  3. Run the build script again (Setup.sh is idempotent and will resume)"
    echo ""
    echo "To check what was completed:"
    echo "  ls -la $UE_SOURCE/LocalBuilds/Engine/Linux/ 2>/dev/null || echo 'Build not started yet'"
    echo "  ls -la $UE_SOURCE/Engine/Binaries/ 2>/dev/null || echo 'No binaries built yet'"
    echo ""
    exit $BUILD_EXIT_CODE
fi

BUILD_EXIT_CODE=0

echo ""
echo "======================================================================="
if [ $BUILD_EXIT_CODE -eq 0 ]; then
    echo "✓ Build completed successfully!"
    echo "======================================================================="
    echo ""
    if [ -n "$UE_OUTPUT" ]; then
        echo "Unreal Engine Linux is now available at:"
        echo "  $UE_OUTPUT"
        echo ""
        echo "Next steps:"
        echo "  1. Set UE_ROOT:"
        echo "     export UE_ROOT=$UE_OUTPUT"
    else
        echo "Unreal Engine Linux is now available at:"
        echo "  $UE_SOURCE/LocalBuilds/Engine/Linux"
        echo ""
        echo "Next steps:"
        echo "  1. Set UE_ROOT:"
        echo "     export UE_ROOT=$UE_SOURCE/LocalBuilds/Engine/Linux"
    fi
    echo ""
    echo "  2. Build Project AirSim Linux binary:"
    echo "     cd CloudMultiPlayerProject/docker/build"
    echo "     ./build_linux_in_docker.sh"
    echo ""
else
    echo "✗ Build failed with exit code: $BUILD_EXIT_CODE"
    echo "======================================================================="
    echo ""
    echo "Check the logs above for errors."
    echo "You can also check container logs:"
    echo "  docker logs ue-linux-builder"
    echo ""
    exit $BUILD_EXIT_CODE
fi

