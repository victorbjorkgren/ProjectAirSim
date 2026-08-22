# Dockerfile for building Unreal Engine Linux from source
# This builds UE 5.7.1 (or compatible) for Linux inside a container
# 
# Usage:
#   docker build -t ue-linux-builder:5.7 -f CloudMultiPlayerProject/docker/build_ue/Dockerfile .
#   docker run --rm -v ~/projects/DroneProject/UE:/ue-source -v ~/ue-output:/ue-output ue-linux-builder:5.7

# Use linux/arm64 platform for native ARM64 Linux builds
# This runs natively on Apple Silicon without emulation, avoiding .NET binary issues
FROM ubuntu:22.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install all dependencies required for building Unreal Engine on Linux
# Based on: https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Build/BatchFiles/Linux/README.md
RUN apt-get update && apt-get install -y --no-install-recommends \
    # Build essentials
    build-essential \
    cmake \
    git \
    git-lfs \
    # Required libraries
    libc6-dev \
    libssl-dev \
    libasound2-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libxext-dev \
    libxfixes-dev \
    libxkbcommon-dev \
    libxkbcommon-x11-dev \
    libx11-dev \
    libx11-xcb-dev \
    libxcb1-dev \
    libxcb-keysyms1-dev \
    libxcb-image0-dev \
    libxcb-shm0-dev \
    libxcb-icccm4-dev \
    libxcb-sync-dev \
    libxcb-xfixes0-dev \
    libxcb-shape0-dev \
    libxcb-randr0-dev \
    libxcb-render-util0-dev \
    libxcb-util-dev \
    libxcb-xinerama0-dev \
    libxcb-dri3-dev \
    libxcb-ewmh-dev \
    libfontconfig1-dev \
    libfreetype6-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libdrm-dev \
    libxss1 \
    libgconf-2-4 \
    # Python (required for UAT)
    python3 \
    python3-pip \
    # Additional tools
    curl \
    wget \
    unzip \
    zip \
    rsync \
    # For monitoring build progress
    htop \
    && rm -rf /var/lib/apt/lists/*

# Install .NET SDK for ARM64 (required for UnrealBuildTool in UE 5.7+)
# UE 5.7 uses .NET instead of Mono for UnrealBuildTool
# Using ARM64 .NET which will run natively without emulation issues
RUN curl -sSL https://dot.net/v1/dotnet-install.sh | bash /dev/stdin --channel 8.0 --architecture arm64 --install-dir /usr/share/dotnet || \
    curl -sSL https://dot.net/v1/dotnet-install.sh | bash /dev/stdin --channel 7.0 --architecture arm64 --install-dir /usr/share/dotnet
ENV PATH="${PATH}:/usr/share/dotnet"
ENV DOTNET_ROOT="/usr/share/dotnet"

# The UE source will be mounted at /ue-source (read-write for build artifacts)
# The output will be written to /ue-output
# We build directly from the mounted source (no copy needed)

# Copy the build script to a standard location
COPY build_ue_linux.sh /usr/local/bin/build_ue_linux.sh
RUN chmod +x /usr/local/bin/build_ue_linux.sh

# Default command
# CMD ["/usr/local/bin/build_ue_linux.sh"]
CMD ["/bin/bash"]
