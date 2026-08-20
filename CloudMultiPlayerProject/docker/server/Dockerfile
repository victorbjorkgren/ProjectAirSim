# Base image matching the user's request (assuming Humble for "Kilted")
FROM ros:humble-ros-base-jammy

# Install system dependencies for Unreal Engine and AirSim
RUN apt-get update && apt-get install -y --no-install-recommends \
    xdg-user-dirs \
    libvulkan1 \
    vulkan-tools \
    libglu1-mesa-dev \
    libxcursor-dev \
    libxrandr-dev \
    libxi-dev \
    libasound2 \
    && rm -rf /var/lib/apt/lists/*

# Create app directory
WORKDIR /app

# Note: The user needs to place the Linux ARM64 build of Blocks here
# We expect the structure: /app/Blocks/Binaries/Linux/Blocks (ARM64 binary)
COPY Blocks /app/Blocks

# Expose AirSim ports
# 41451: RPC port
# 41452: Video stream port (optional)
EXPOSE 41451 41452

# Create non-root user (Unreal often complains if run as root, though in Docker it might be fine)
# For now running as root for simplicity, can secure later in Phase 11
# USER root

# Set environment variables for Headless mode
ENV SDL_VIDEODRIVER=offscreen
ENV DISPLAY=

# Launch command
# -RenderOffScreen : Runs without a window
# -nullrhi : Disables rendering hardware interface (for pure physics/API) - CAUTION: Camera APIs might fail
# Usually we want -RenderOffScreen but keep RHI for camera images if GPU is present
# If running on CPU only (no GPU), use -nullrhi but then no images.
# Assuming GPU access via nvidia-container-runtime for cameras.
CMD ["/app/Blocks/Binaries/Linux/Blocks", "-RenderOffScreen", "-windowed", "-ResX=640", "-ResY=480"]


