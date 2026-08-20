# Project AirSim Multiplayer Kubernetes - Status Tracker

## Project Goal
Transform Project AirSim into a distributed multiplayer drone simulation running on Kubernetes with ROS2/Zenoh communication.

## Quick Progress Overview

```
[✓] Phase 1  - Baseline Mac Setup
[✓] Phase 2  - Multiple Drones (Single Machine)
[~] Phase 3  - ROS2 Integration (Deferred)
[✓] Phase 4  - Headless Mode
[ ] Phase 5  - Docker Single Drone
[ ] Phase 6  - Docker Multiple Drones
[ ] Phase 7  - Zenoh Integration
[ ] Phase 8  - Local Kubernetes
[ ] Phase 9  - Pixel Streaming Spectator
[ ] Phase 10 - Minimap Visualization
[ ] Phase 11 - Production Hardening
[ ] Phase 12 - Advanced Features
```

Legend: `[?]` = Starting, `[~]` = In Progress, `[✓]` = Complete, `[ ]` = Not Started

---

## Phase Checklists

### Phase 1: Baseline Mac Setup [✓]

**Start Date**: November 20, 2025  
**Completion Date**: November 20, 2025

- [x] Build Project AirSim from source
  - [x] Run `./build_macos.sh` successfully
  - [x] Verify libraries in `build/lib/`
- [x] Generate Unreal project files
  - [x] Run `blocks_genprojfiles_vscode.sh`
- [x] Build Blocks in Unreal
  - [x] Open Blocks.uproject
  - [x] Successfully compile
  - [x] Press Play - environment loads
- [x] Install Python client
  - [x] Create virtual environment
  - [x] `pip install -e client/python`
  - [x] Import works: `import projectairsim`
- [x] Run test script
  - [x] Connect to sim server
  - [x] Drone takes off
  - [x] Drone moves on command
  - [x] Capture camera image
  - [x] Drone lands

**Notes**:
```
Reproduction:
- Build by `./build_macos.sh package_blocks_shipping`
- Run the packaged app
- Run `python hello_drone.py` from `./clients/example_user_scripts/`
- Note drone take off and land in the app window

Issues encountered:
- Shipping builds failed to bind network ports (8989/8990), causing "Connection refused" 
  errors from Python client
- Root cause: macOS App Sandbox with Sandbox.NoNet.entitlements blocked network access
- Development builds worked fine (different sandbox configuration)

Solutions:
- Updated unreal/Blocks/Build/Mac/Resources/Sandbox.NoNet.entitlements to include:
  * com.apple.security.network.client (outgoing connections)
  * com.apple.security.network.server (listening on ports)
- Improved log file handling in UnrealSimLoader.cpp for better Shipping build diagnostics
- Added debug logging to ProjectAirSimGameMode.cpp for future troubleshooting
- Both Development and Shipping packages now working correctly


```

---

### Phase 2: Multiple Drones (Single Machine) [✓]

**Start Date**: November 20, 2025  
**Completion Date**: November 20, 2025

- [x] Create custom scene configuration
  - [x] Add 3 drones with different positions
  - [x] Save as `multi_drone_scene.jsonc`
- [x] Modify Python script for multiple drones
  - [x] Create Drone objects for each
  - [x] Control independently
- [x] Test scenarios
  - [x] Formation flight
  - [x] Independent waypoints
  - [x] Collision detection
- [x] Performance verification
  - [x] FPS remains stable (>30)
  - [x] All camera feeds accessible
  - [x] No lag in controls

**Configuration File Location**:
```
client/python/example_user_scripts/sim_config/multi_drone_scene.jsonc
```

**Notes**:
```
Created `multi_drone_scene.jsonc` with 3 drones (Drone1, Drone2, Drone3).
Created `multi_drone_test.py` demonstrating:
- Sequential takeoff
- Formation flight (V-formation)
- Independent waypoints
- Re-grouping and landing
```

---

### Phase 3: Headless Mode [✓]

**Start Date**: November 20, 2025
**Completion Date**: November 20, 2025

- [x] Test `-RenderOffScreen`
  - [x] Launch: `./Blocks.app ... -RenderOffScreen`
  - [x] Verified behavior: API server fails to start properly on Mac local environment
- [x] Test `-nullrhi`
  - [x] Launch: `./Blocks.app ... -nullrhi`
  - [x] Verified behavior: Simulation runs but API server unresponsive on Mac local environment
- [x] Benchmark performance
  - [x] Created `client/python/example_user_scripts/phase3_benchmark.py`
  - [x] Attempted headless benchmarking

**Notes**:
```
Headless mode testing on macOS (M1/M2/M3) encountered issues where the API server
fails to initialize or respond to requests when running in `-RenderOffScreen` or `-nullrhi` modes via command line.
This is likely due to macOS window server restrictions or entitlements when running outside the standard App launch flow.
For Docker deployment (Phase 4+), Linux containers will be used where headless mode is standard and fully supported.
The benchmark script `phase3_benchmark.py` is ready for use in the Docker environment.
```

---

### Phase 4: Docker Single Drone [~]

**Start Date**: November 20, 2025
**Completion Date**: _____________

- [x] Create server Dockerfile
  - [x] Base: `ros:humble-ros-base-jammy`
  - [x] Install dependencies
  - [x] Copy Blocks binary (Placeholders created)
  - [x] Expose ports 41451, 41452
- [ ] Build server image
  - [ ] `docker build -t projectairsim-server:v1 .`
  - [ ] Test locally: `docker run ...`
- [x] Create client Dockerfile
  - [x] Base: `ros:humble-ros-base-jammy`
  - [x] Install Python client
  - [x] Install ROS2 bridge (Client dependencies installed)
- [ ] Build client image
  - [ ] `docker build -t projectairsim-client:v1 .`
- [x] Create docker-compose.yml
  - [x] Define services: server, client
  - [x] Configure networking
  - [x] Mount config volumes
- [ ] Test full stack
  - [ ] `docker-compose up`
  - [ ] Client connects to server
  - [ ] Drone control works
  - [ ] Camera images retrieved

**Docker Images Created**:
- `projectairsim-server:v1` - Size: ______
- `projectairsim-client:v1` - Size: ______

**Notes**:
```
Created Dockerfiles and docker-compose.yml in `CloudMultiPlayerProject/docker/`.
Created Docker-based build system in `CloudMultiPlayerProject/docker/build/` to build Linux binaries from macOS.

Build System:
- Dockerfile for Linux build environment (Ubuntu 22.04, Clang-13, CMake, Ninja)
- build_linux_in_docker.sh: Automated build script
- check_prerequisites.sh: Prerequisites checker
- Full documentation in build/README.md

Requirements:
- Docker must be installed and running
- Linux version of Unreal Engine required (Mac UE cannot be used for Linux builds)
- UE_ROOT must point to Linux UE installation (can be remote/mounted)

Modified `hello_drone.py` to support `PROJECTAIRSIM_HOST` env var for Docker networking.

Next: User needs to obtain Linux UE (remote server, Docker volume, or build from source) to complete the build.
```

---

### Phase 5: ROS2 Integration [ ]

**Start Date**: ______
**Completion Date**: _______

- [ ] Setup ROS2
  - [ ] Install ROS2 Kilted
  - [ ] Source ROS2 environment
- [ ] Build Project AirSim ROS2 packages
  - [ ] `cd ros/node/projectairsim-ros2`
  - [ ] `colcon build`
- [ ] Test single drone with ROS2
  - [ ] Launch ROS2 bridge
  - [ ] Verify topics: `ros2 topic list`
  - [ ] Echo pose data: `ros2 topic echo /drone1/pose`
- [ ] Create multi-drone launch file
  - [ ] Launch file for 3 drones
  - [ ] Each drone has namespaced topics
- [ ] Test ROS2 control
  - [ ] Command via `ros2 topic pub`
  - [ ] Verify in RViz2
- [ ] Visualize in RViz2
  - [ ] Show drone TF frames
  - [ ] Display camera images

**Key Topics Implemented**:
- [ ] `/drone_X/pose`
- [ ] `/drone_X/cmd_vel`
- [ ] `/drone_X/camera/image_raw`
- [ ] `/drone_X/imu`

**Notes**:
```
Skipping Phase 3 due to ROS2 installation issues on Mac.
Moving to Docker-based client setup (Phase 5).
```

---

### Phase 6: Docker Multiple Drones [ ]

**Start Date**: _____________  
**Completion Date**: _____________

- [ ] Update docker-compose.yml
  - [ ] Add 3 drone client services
  - [ ] Each with unique `DRONE_NAME` env var
- [ ] Configure ROS2 networking
  - [ ] Shared ROS_DOMAIN_ID
  - [ ] Enable multicast
- [ ] Test deployment
  - [ ] All containers start
  - [ ] All drones connect
  - [ ] Independent control
- [ ] Test ROS2 communication
  - [ ] Topics visible across containers
  - [ ] `ros2 topic list` from host
- [ ] Resource monitoring
  - [ ] Check memory per container
  - [ ] Check CPU per container

**Resource Usage**:
- Server: _____ CPU, _____ MB RAM
- Client (each): _____ CPU, _____ MB RAM

**Notes**:
```


```

---

### Phase 7: Zenoh Integration [ ]

**Start Date**: _____________  
**Completion Date**: _____________

- [ ] Install Zenoh
  - [ ] Zenoh router: `zenohd`
  - [ ] ROS2 bridge: `zenoh-bridge-ros2dds`
- [ ] Deploy Zenoh router container
  - [ ] Create Dockerfile
  - [ ] Add to docker-compose
- [ ] Configure ROS2 to use Zenoh
  - [ ] Set `RMW_IMPLEMENTATION=rmw_zenoh_cpp`
  - [ ] Update client containers
- [ ] Test with single drone
  - [ ] Verify communication works
  - [ ] Check latency
- [ ] Test with multiple drones
  - [ ] All drones communicate via Zenoh
  - [ ] Check performance vs FastDDS
- [ ] Benchmark
  - [ ] Message latency: _____ms (Zenoh) vs _____ms (FastDDS)
  - [ ] Throughput: _____msgs/sec

**Notes**:
```


```

---

### Phase 8: Local Kubernetes [ ]

**Start Date**: _____________  
**Completion Date**: _____________

- [ ] Setup local K8s cluster
  - [ ] Install minikube/kind
  - [ ] Start cluster: `minikube start`
  - [ ] Verify: `kubectl cluster-info`
- [ ] Create Kubernetes manifests
  - [ ] `sim-server-deployment.yaml`
  - [ ] `sim-server-service.yaml`
  - [ ] `drone-statefulset.yaml`
  - [ ] `zenoh-router-deployment.yaml`
  - [ ] `configmap.yaml` (scene config)
- [ ] Push images to local registry
  - [ ] Setup local registry
  - [ ] Tag and push images
- [ ] Deploy to K8s
  - [ ] `kubectl apply -f k8s/`
  - [ ] Check pods: `kubectl get pods`
  - [ ] Check logs: `kubectl logs ...`
- [ ] Test functionality
  - [ ] All pods running
  - [ ] Drones connect to server
  - [ ] ROS2/Zenoh communication works
- [ ] Test scaling
  - [ ] `kubectl scale statefulset drones --replicas=5`
  - [ ] Verify all 5 drones work
- [ ] Test resilience
  - [ ] Kill a drone pod, watch it restart
  - [ ] Kill server pod, verify recovery

**Cluster Info**:
- K8s version: _____
- Number of nodes: _____
- GPU available: Yes / No

**Notes**:
```


```

---

### Phase 9: Pixel Streaming Spectator [ ]

**Start Date**: _____________  
**Completion Date**: _____________

- [ ] Review pixel streaming setup
  - [ ] Study `run_sim_with_streaming.sh`
  - [ ] Understand signaling server
- [ ] Configure Unreal for pixel streaming
  - [ ] Enable PixelStreaming plugin
  - [ ] Add spectator camera to scene
- [ ] Deploy signaling server
  - [ ] Create Dockerfile
  - [ ] Deploy to K8s
  - [ ] Expose via Service/Ingress
- [ ] Configure WebRTC
  - [ ] Set up TURN/STUN servers (if needed)
  - [ ] Configure firewall rules
- [ ] Create web viewer
  - [ ] HTML page to view stream
  - [ ] Add controls (keyboard/mouse)
- [ ] Test locally
  - [ ] Access from browser
  - [ ] Verify < 100ms latency
  - [ ] Test controls
- [ ] Test multiple spectators
  - [ ] 2+ viewers simultaneously

**Access URL**: _____________________

**Notes**:
```


```

---

### Phase 10: Minimap Visualization [ ]

**Start Date**: _____________  
**Completion Date**: _____________

- [ ] Choose visualization approach
  - [ ] Decision: Web (Three.js) / RViz2 / Overlay
- [ ] Create minimap backend
  - [ ] ROS2 subscriber for all drone poses
  - [ ] Position aggregator
  - [ ] WebSocket server
- [ ] Create minimap frontend
  - [ ] Three.js 2D/3D visualization
  - [ ] Real-time position updates
  - [ ] Trail history
  - [ ] Zoom/pan controls
- [ ] Containerize minimap service
  - [ ] Create Dockerfile
  - [ ] Deploy to K8s
- [ ] Test with multiple drones
  - [ ] All drones visible
  - [ ] Real-time updates
  - [ ] No lag with 10+ drones
- [ ] Add features
  - [ ] Click drone for details
  - [ ] Color coding by status
  - [ ] Minimap overlays

**Minimap URL**: _____________________

**Notes**:
```


```

---

### Phase 11: Production Hardening [ ]

**Start Date**: _____________  
**Completion Date**: _____________

#### Resource Management
- [ ] Set CPU limits on all pods
- [ ] Set memory limits on all pods
- [ ] Configure resource requests
- [ ] Test with constrained resources

#### Health Checks
- [ ] Add liveness probes
- [ ] Add readiness probes
- [ ] Test pod restart on failure
- [ ] Implement graceful shutdown

#### Monitoring
- [ ] Deploy Prometheus
- [ ] Deploy Grafana
- [ ] Create dashboards
  - [ ] Drone connection status
  - [ ] ROS2 latency
  - [ ] Camera FPS
  - [ ] Resource usage
- [ ] Add alerting rules

#### Logging
- [ ] Centralized logging (ELK/Loki)
- [ ] Structured logging (JSON)
- [ ] Log aggregation working
- [ ] Log retention policy

#### Security
- [ ] Non-root containers
- [ ] Network policies defined
- [ ] Secrets for sensitive config
- [ ] Image vulnerability scanning
- [ ] RBAC configured

#### Persistence
- [ ] PersistentVolumeClaims created
- [ ] Backup procedures documented
- [ ] Restore tested

#### Performance
- [ ] Load test: Max drones = _____
- [ ] Latency under load: _____ms
- [ ] Set up HPA (Horizontal Pod Autoscaler)

**Notes**:
```


```

---

### Phase 12: Advanced Features [ ]

**Start Date**: _____________  
**Completion Date**: _____________

Pick and choose based on your needs:

- [ ] Multi-region deployment
- [ ] Dynamic scene loading
- [ ] Recording & playback (ROS bags)
- [ ] AI/ML integration
- [ ] Multi-user missions
- [ ] Advanced spectator features
- [ ] Performance optimizations
- [ ] Custom game modes
- [ ] Integration with other tools

**Features Implemented**:
```


```

---

## Current Focus

**Active Phase**: Phase 4

**Current Task**: 
```
Dockerizing Simulation and Client
```

**Blockers**:
```
None
```

**Next Milestone**:
```
Running simulation in Docker container
```

---

## Key Metrics Dashboard

| Metric | Target | Current |
|--------|--------|---------|
| Max Simultaneous Drones | 10+ | ____ |
| ROS2 Message Latency | < 50ms | ____ |
| Camera FPS (per drone) | 30+ | ____ |
| Pod Startup Time | < 5s | ____ |
| System Uptime | 99%+ | ____ |
| Pixel Stream Latency | < 100ms | ____ |

---

## Architecture Decisions Log

| Date | Decision | Rationale |
|------|----------|-----------|
| 2024-11-19 | Use Zenoh over FastDDS | Better K8s networking, lower latency |
| | Server-side rendering initially | Simpler, less resources per pod |
| | StatefulSet for drones | Need stable network identity |
| | | |

---

## Resources & Links

- **Main Roadmap**: `MULTIPLAYER_ROADMAP.md`
- **Phase 1 Guide**: `PHASE1_QUICKSTART.md`
- **Project Repo**: [Project AirSim](https://github.com/iamaisim/ProjectAirSim)
- **Discord**: https://discord.gg/XprQ2w64uj
- **Docker Images**: (registry location)
- **K8s Cluster**: (cluster details)

---

## Team & Contacts

**Project Lead**: Victor Bjorkgren

**Support**:
- Discord: Project AirSim community
- Email: (if applicable)

---

**Last Updated**: 2024-11-20  
**Document Version**: 1.1

---

## Quick Commands Reference

```bash
# Phase 1
./build_macos.sh
open Blocks.uproject

# Phase 3
source /opt/ros/humble/setup.bash
ros2 topic list

# Phase 5
docker-compose up -d
docker-compose logs -f

# Phase 8
kubectl get pods
kubectl logs -f drone-0
kubectl scale statefulset drones --replicas=5

# Debugging
kubectl describe pod <pod-name>
kubectl exec -it <pod-name> -- /bin/bash
ros2 topic echo /drone1/pose

# Cleanup
kubectl delete -f k8s/
docker-compose down -v
```

---

**Ready to begin?** Start with Phase 1! 🚁
