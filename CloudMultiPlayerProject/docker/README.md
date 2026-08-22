# Project AirSim Docker Deployment

This directory contains the Docker setup for running Project AirSim (Simulation and Client) in containers.

## Prerequisites

1. **Docker & Docker Compose**: Ensure these are installed.
2. **Linux Binary of Project AirSim**:
   Since the simulation (Blocks) runs on Linux in the container, you must provide the Linux build of the project.
   
   **Option A: Build from macOS** (Recommended):
   - **Step 1**: Build Unreal Engine Linux (if you don't have it):
     - See `build_ue/README.md` for building UE Linux from source
     - Run: `cd build_ue && ./build_ue_linux_from_mac.sh`
     - This takes 4+ hours and ~200GB disk space
   - **Step 2**: Build Project AirSim Linux binary:
     - See `build/README.md` for instructions
     - Run: `cd build && ./build_linux_in_docker.sh`
     - The output will automatically be placed in `server/Blocks/`
   
   **Option B: Manual Placement**:
   - Build or download the Linux binary of Project AirSim on a Linux machine
   - Place the `Blocks` directory into `CloudMultiPlayerProject/docker/server/`
   - The structure should look like:
     ```
     CloudMultiPlayerProject/docker/server/Blocks/
       ├── Binaries/
       │   └── Linux/
       │       └── Blocks
       ├── Content/
       ├── ...
     ```

## Directory Structure

- `server/`: Contains `Dockerfile` for the Simulation server.
- `client/`: Contains `Dockerfile` for the Python client.
- `docker-compose.yml`: Orchestrates the server and client containers.

## Usage

1. **Prepare the Binary**:
   Ensure `CloudMultiPlayerProject/docker/server/Blocks` exists as described above.

2. **Build and Run**:
   From the `CloudMultiPlayerProject/docker` directory:
   ```bash
   docker-compose up --build
   ```

3. **Verify**:
   - The `server` container will start the simulation in headless mode.
   - The `client` container will wait for the server to be ready, then run `hello_drone.py`.
   - You should see logs indicating the drone is taking off and moving.

## Configuration

- **Headless Mode**: The server runs with `-RenderOffScreen` by default.
- **Client Script**: The client runs `hello_drone.py` by default. You can change the command in `docker-compose.yml`.
- **Networking**: The containers communicate over a bridge network `sim-network`. The server is accessible at hostname `server`.

## Troubleshooting

- **"Blocks not found"**: You forgot to place the Linux binary in `server/Blocks`.
- **"Connection refused"**: The server might still be initializing. The client script waits for port 41451 using `netcat`.
- **Graphics/GPU**:
  - Ensure you have `nvidia-container-toolkit` installed if you want GPU acceleration (required for rendering images even in headless mode usually).
  - Uncomment `runtime: nvidia` in `docker-compose.yml` if available.

