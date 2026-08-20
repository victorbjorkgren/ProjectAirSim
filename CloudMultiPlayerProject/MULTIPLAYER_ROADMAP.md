# Project AirSim Multiplayer Kubernetes Deployment Roadmap

## Project Vision
Transform Project AirSim Blocks environment into a distributed multiplayer drone simulation where:
- Multiple drone clients run as independent Kubernetes pods
- Each client handles its own camera rendering
- A central server maintains overall simulation state
- ROS2 nodes communicate via Zenoh DDS middleware
- Headless operation with optional spectator mode (pixel streaming or direct client)
- Minimap showing drone positions in real-time

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    Kubernetes Cluster                        │
│                                                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ Drone Pod 1  │  │ Drone Pod 2  │  │ Drone Pod N  │     │
│  │              │  │              │  │              │     │
│  │ ROS2 Node    │  │ ROS2 Node    │  │ ROS2 Node    │     │
│  │ Camera Render│  │ Camera Render│  │ Camera Render│     │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘     │
│         │                 │                 │              │
│         └─────────────────┴─────────────────┘              │
│                           │                                 │
│                      Zenoh DDS                              │
│                           │                                 │
│         ┌─────────────────┴─────────────────┐              │
│         │                                   │              │
│  ┌──────▼──────────┐              ┌─────────▼──────┐      │
│  │  Sim Server Pod │              │ Spectator Pod  │      │
│  │                 │              │ (Pixel Stream) │      │
│  │ State Manager   │              │                │      │
│  │ Physics/Collide │              │ Minimap        │      │
│  └─────────────────┘              └────────────────┘      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## Incremental Experiment Phases

### Phase 1: Baseline - Local Mac Setup ✓
**Goal**: Get Project AirSim Blocks running on your Mac with basic Python client control

**Steps**:
1. Build Project AirSim from source on macOS
2. Launch Blocks environment
3. Run a simple Python client script to control a single drone
4. Verify camera feeds are working
5. Test basic movement commands (takeoff, move, land)

**Success Criteria**:
- Blocks environment opens and renders
- Python client connects to simulation server
- Drone responds to API commands
- Camera images can be retrieved

**Documentation Reference**:
- `docs/development/use_source.md`
- `docs/system_specs.md`
- `README_macos.md`

**Time Estimate**: 1-2 days

---

### Phase 2: Multiple Drones - Single Machine
**Goal**: Configure and control multiple drones simultaneously from a single Python client

**Steps**:
1. Create/modify scene configuration with multiple drone actors
2. Update scene JSONC file to spawn 2-3 drones at different positions
3. Modify Python client to connect to multiple drones by name
4. Implement basic formation flying or independent waypoint following
5. Test collision detection between drones

**Success Criteria**:
- Multiple drones visible in simulation
- Independent control of each drone
- No performance degradation with 3 drones
- Camera feeds from all drones accessible

**Key Files**:
- Create custom scene config (based on examples in `client/python/`)
- Refer to `docs/multiple_robots.md`
- Modify actor configuration in scene JSONC

**Testing Script Ideas**:
- Formation flight (triangle, line)
- Independent waypoint missions
- Follow-the-leader behavior

**Time Estimate**: 2-3 days

---

### Phase 3: ROS2 Integration - Single Machine
**Goal**: Run multiple drones with ROS2 nodes publishing/subscribing to topics

**Steps**:
1. Set up ROS2 workspace (if not already done)
2. Build Project AirSim ROS2 packages
3. Launch ROS2 bridge for single drone
4. Verify topics are publishing (pose, IMU, camera, etc.)
5. Create launch file for multiple drone ROS2 nodes
6. Test commanding drones via ROS2 topics instead of direct API
7. Visualize in RViz2

**Success Criteria**:
- Each drone has independent ROS2 node
- Topics namespaced per drone (e.g., `/drone1/camera`, `/drone2/pose`)
- Can command drones via ROS2 messages
- RViz2 shows drone positions and orientations

**Key Files**:
- `ros/node/projectairsim-ros2/`
- Create custom launch files in `ros/examples/`
- Reference existing ROS examples

**ROS2 Topics to Implement**:
- `/drone_X/cmd_vel` - velocity commands
- `/drone_X/pose` - position/orientation
- `/drone_X/camera/image_raw` - camera feed
- `/drone_X/imu` - IMU data
- `/drone_X/gps` - GPS data

**Time Estimate**: 3-5 days

---

### Phase 4: Headless Mode Testing
**Goal**: Run simulation without GUI, prepare for containerization

**Steps**:
1. Test `-RenderOffScreen` mode with single drone
2. Verify camera rendering works in offscreen mode
3. Test `-nullrhi` mode (no rendering at all)
4. Benchmark performance differences (headless vs GUI)
5. Ensure ROS2 bridge works in headless mode
6. Test remote client connection (from different terminal/machine)

**Success Criteria**:
- Simulation runs without display
- Camera images still captured in offscreen mode
- Stable performance over extended runs
- Remote client can connect and control

**Command Examples**:
```bash
# Offscreen rendering (camera images work)
./Blocks.sh -RenderOffScreen

# No rendering (faster, no images)
./Blocks.sh -nullrhi
```

**Documentation Reference**:
- `docs/development/headless_cloud.md`
- `README.md` (Headless section)

**Time Estimate**: 2-3 days

---

### Phase 5: Docker Containerization - Single Drone
**Goal**: Create Docker images for simulation server and drone client

**Steps**:
1. Create Dockerfile for simulation server
   - Base: Ubuntu 22.04
   - Install Unreal Engine dependencies
   - Copy Blocks binary and assets
   - Expose necessary ports
2. Create Dockerfile for drone client
   - Base: ROS2 Humble/Iron
   - Install Project AirSim Python client
   - Install ROS2 bridge packages
3. Test single drone in Docker on local machine
4. Verify networking between containers (docker-compose)
5. Test volume mounts for configuration files

**Success Criteria**:
- Simulation server runs in container
- Drone client container connects to server
- ROS2 topics accessible from host
- Configuration can be changed without rebuilding

**Docker Architecture**:
```yaml
# docker-compose.yml structure
services:
  sim-server:
    image: projectairsim-server
    ports:
      - "41451:41451"  # RPC port
      - "41452:41452"  # Topics port
    volumes:
      - ./config:/config
      
  drone-client:
    image: projectairsim-ros2-client
    depends_on:
      - sim-server
    environment:
      - SIM_SERVER=sim-server
      - DRONE_NAME=Drone1
```

**Challenges to Address**:
- GPU access in container (NVIDIA Container Toolkit)
- Display forwarding (X11) for testing
- Network performance (host vs bridge)

**Time Estimate**: 4-6 days

---

### Phase 6: Multiple Containerized Drone Clients
**Goal**: Run multiple drone clients as separate containers

**Steps**:
1. Modify docker-compose to spawn multiple drone containers
2. Each container controls one drone
3. Implement environment variable configuration:
   - `DRONE_NAME`
   - `SIM_SERVER_HOST`
   - `ROS_DOMAIN_ID`
4. Test inter-container ROS2 communication
5. Implement shared ROS2 network (multicast)
6. Verify each drone's camera rendering in its own container

**Success Criteria**:
- 3+ drone containers running simultaneously
- Each drone independently controlled
- ROS2 topics visible across all containers
- Minimal resource overhead per container

**Docker Compose Example**:
```yaml
services:
  sim-server:
    # ... as before ...
    
  drone1:
    image: projectairsim-ros2-client
    environment:
      - DRONE_NAME=Drone1
      - SIM_SERVER=sim-server
      
  drone2:
    image: projectairsim-ros2-client
    environment:
      - DRONE_NAME=Drone2
      - SIM_SERVER=sim-server
      
  drone3:
    image: projectairsim-ros2-client
    environment:
      - DRONE_NAME=Drone3
      - SIM_SERVER=sim-server
```

**Time Estimate**: 3-4 days

---

### Phase 7: Zenoh DDS Middleware Integration
**Goal**: Replace default DDS with Zenoh for better scalability and cloud-native operation

**Steps**:
1. Install Zenoh and Zenoh-DDS bridge
2. Configure ROS2 to use Zenoh as RMW (ROS Middleware)
3. Test with single drone
4. Benchmark performance vs FastDDS/CycloneDDS
5. Deploy Zenoh router as separate container
6. Configure all drone clients to use Zenoh router
7. Test pub-sub patterns with multiple drones

**Success Criteria**:
- ROS2 nodes communicate via Zenoh
- Lower latency than default DDS
- Better scalability with 5+ drones
- Zenoh router handles message routing

**Configuration**:
```bash
# Set Zenoh as RMW
export RMW_IMPLEMENTATION=rmw_zenoh_cpp

# Zenoh router config
zenohd --config zenoh-router-config.json5
```

**Why Zenoh?**
- Better suited for Kubernetes networking
- Built-in discovery across networks
- Lower overhead than DDS multicast
- Cloud-native design

**Documentation**:
- [Zenoh ROS2 Integration](https://github.com/eclipse-zenoh/zenoh-plugin-ros2dds)
- [Zenoh Docker Setup](https://zenoh.io/docs/getting-started/deployment/)

**Time Estimate**: 4-5 days

---

### Phase 8: Local Kubernetes Deployment (Minikube/Kind)
**Goal**: Deploy the full system to a local Kubernetes cluster

**Steps**:
1. Set up local Kubernetes cluster (minikube or kind)
2. Create Kubernetes manifests:
   - Deployment for sim server
   - StatefulSet for drone clients (scalable)
   - Service for sim server exposure
   - ConfigMap for scene configuration
   - NetworkPolicy for inter-pod communication
3. Push Docker images to local registry
4. Deploy sim server first, wait for ready
5. Deploy drone clients (start with 2)
6. Test DNS-based service discovery
7. Scale drone clients up and down dynamically

**Success Criteria**:
- All pods running and healthy
- Drones connect to sim server via Kubernetes service
- Can scale drone replicas: `kubectl scale statefulset drones --replicas=5`
- ROS2/Zenoh communication works across pods
- Logs accessible via kubectl

**Kubernetes Manifest Structure**:
```yaml
# sim-server-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: sim-server
spec:
  replicas: 1
  template:
    spec:
      containers:
      - name: server
        image: projectairsim-server:latest
        ports:
        - containerPort: 41451
        - containerPort: 41452
---
# sim-server-service.yaml
apiVersion: v1
kind: Service
metadata:
  name: sim-server
spec:
  selector:
    app: sim-server
  ports:
  - name: rpc
    port: 41451
  - name: topics
    port: 41452
---
# drone-statefulset.yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: drones
spec:
  serviceName: "drones"
  replicas: 3
  template:
    spec:
      containers:
      - name: drone-client
        image: projectairsim-ros2-client:latest
        env:
        - name: DRONE_NAME
          value: "Drone$(POD_INDEX)"
        - name: SIM_SERVER
          value: "sim-server.default.svc.cluster.local"
```

**Networking Considerations**:
- Use CNI plugin that supports multicast (if not using Zenoh router)
- Or deploy Zenoh router as a service
- Consider using host networking for ROS2 (if needed)

**Testing**:
```bash
# Deploy
kubectl apply -f k8s/

# Check status
kubectl get pods
kubectl logs -f sim-server-xxx
kubectl logs -f drones-0

# Scale drones
kubectl scale statefulset drones --replicas=5

# Port forward to access from host
kubectl port-forward svc/sim-server 41451:41451
```

**Time Estimate**: 5-7 days

---

### Phase 9: Spectator Mode - Pixel Streaming
**Goal**: Enable remote viewing of the simulation with fly-around camera

**Steps**:
1. Review existing pixel streaming setup (`run_sim_with_streaming.sh`)
2. Configure Unreal Engine pixel streaming plugin for Blocks
3. Deploy signaling server as Kubernetes pod
4. Add spectator camera to Blocks scene (free-fly or orbit mode)
5. Configure WebRTC for streaming
6. Create web interface to view stream
7. Test on external device (laptop, tablet)
8. Add keyboard/mouse controls for spectator

**Success Criteria**:
- Can view simulation from browser
- Low latency (< 100ms)
- Spectator camera can fly around independently
- Does not affect drone simulation performance
- Multiple spectators can watch simultaneously

**Architecture**:
```
┌─────────────┐     WebRTC      ┌──────────────────┐
│   Browser   │ ◄─────────────► │  Signaling Server│
└─────────────┘                 └──────────────────┘
                                         │
                                         │ WebSocket
                                         │
                                ┌────────▼─────────┐
                                │   Sim Server     │
                                │ (Pixel Streaming)│
                                └──────────────────┘
```

**Pixel Streaming Setup**:
```bash
# In Kubernetes
kubectl apply -f k8s/signaling-server.yaml

# Access via ingress or NodePort
https://simulation.yourdomain.com/
```

**Documentation Reference**:
- `run_sim_with_streaming.sh`
- [Unreal Pixel Streaming Docs](https://docs.unrealengine.com/5.6/en-US/pixel-streaming-in-unreal-engine/)

**Alternative Approach** (simpler):
- Deploy a standalone Unreal client in spectator mode
- No rendering on server, spectator connects like a regular client
- May need custom game mode in Blocks

**Time Estimate**: 5-7 days

---

### Phase 10: Minimap Visualization
**Goal**: Add real-time minimap showing all drone positions

**Steps**:
1. Choose visualization approach:
   - **Option A**: Web-based (Three.js, Cesium)
   - **Option B**: ROS2 RViz2 plugin
   - **Option C**: Overlayed on pixel stream
2. Aggregate drone poses from all ROS2 topics
3. Create 2D top-down visualization
4. Add trail history (last N positions)
5. Color-code drones
6. Add zoom/pan controls
7. Deploy as separate pod with web interface

**Success Criteria**:
- Real-time position updates (< 100ms latency)
- Shows all active drones
- Clickable drone markers (show details)
- Accessible via web browser
- Scales to 10+ drones

**Minimap Architecture**:
```
┌──────────────────────────────────────────┐
│         Minimap Service                   │
│  ┌────────────────────────────────────┐  │
│  │  ROS2 Subscriber                   │  │
│  │  (listens to /drone_*/pose)        │  │
│  └────────────┬───────────────────────┘  │
│               │                           │
│  ┌────────────▼───────────────────────┐  │
│  │  Position Aggregator               │  │
│  │  (maintains state of all drones)   │  │
│  └────────────┬───────────────────────┘  │
│               │                           │
│  ┌────────────▼───────────────────────┐  │
│  │  WebSocket Server                  │  │
│  └────────────┬───────────────────────┘  │
└───────────────┼───────────────────────────┘
                │ WebSocket
        ┌───────▼────────┐
        │  Web Browser   │
        │  (Three.js map)│
        └────────────────┘
```

**Tech Stack Options**:

**Option A - Lightweight Web**:
```typescript
// Simple Three.js visualization
const drones = new Map();

ws.onmessage = (event) => {
  const pose = JSON.parse(event.data);
  updateDronePosition(pose.drone_name, pose.x, pose.y, pose.z);
};
```

**Option B - RViz2 Plugin**:
- More powerful, 3D visualization
- Requires ROS2 knowledge
- Better for debugging

**Deployment**:
```yaml
# minimap-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: minimap
spec:
  template:
    spec:
      containers:
      - name: minimap-service
        image: projectairsim-minimap:latest
        ports:
        - containerPort: 8080  # WebSocket
        - containerPort: 80    # HTTP (web UI)
---
apiVersion: v1
kind: Service
metadata:
  name: minimap
spec:
  type: LoadBalancer  # or Ingress
  ports:
  - name: web
    port: 80
  - name: ws
    port: 8080
```

**Time Estimate**: 4-6 days

---

### Phase 11: Production Kubernetes Hardening
**Goal**: Prepare system for production deployment with reliability and scalability

**Steps**:
1. **Resource Management**:
   - Set CPU/memory limits on all pods
   - Configure resource requests
   - Test with resource constraints
   
2. **Health Checks**:
   - Add liveness probes (is pod alive?)
   - Add readiness probes (is pod ready for traffic?)
   - Implement graceful shutdown
   
3. **Monitoring & Observability**:
   - Deploy Prometheus for metrics
   - Add Grafana dashboards
   - Instrument ROS2 nodes with metrics
   - Add distributed tracing (Jaeger/Zipkin)
   
4. **Logging**:
   - Centralized logging (ELK or Loki stack)
   - Structured logging (JSON format)
   - Log aggregation from all pods
   
5. **Security**:
   - Non-root containers
   - Network policies (restrict inter-pod traffic)
   - Secrets management (for configs)
   - Image scanning
   
6. **Persistence**:
   - PersistentVolumeClaims for logs/data
   - Backup/restore procedures
   - Configuration version control
   
7. **Scaling & Performance**:
   - Horizontal Pod Autoscaler (HPA) for drones
   - Load testing (max drones per server)
   - Multi-server architecture (if needed)

**Success Criteria**:
- All pods have health checks
- Monitoring dashboard shows key metrics
- Logs centrally aggregated and searchable
- System survives pod crashes (auto-restart)
- Can scale to 20+ drones
- Security scan shows no critical vulnerabilities

**Key Metrics to Monitor**:
- Drone connection status
- ROS2 message latency
- Camera frame rate
- CPU/memory per pod
- Network throughput
- Physics simulation step time

**Example Resource Limits**:
```yaml
resources:
  requests:
    memory: "512Mi"
    cpu: "500m"
  limits:
    memory: "2Gi"
    cpu: "2000m"
```

**Example Probes**:
```yaml
livenessProbe:
  httpGet:
    path: /health
    port: 8080
  initialDelaySeconds: 30
  periodSeconds: 10

readinessProbe:
  httpGet:
    path: /ready
    port: 8080
  initialDelaySeconds: 5
  periodSeconds: 5
```

**Time Estimate**: 7-10 days

---

### Phase 12: Advanced Features & Optimization
**Goal**: Add nice-to-have features and optimize performance

**Potential Additions**:

1. **Multi-Region Deployment**:
   - Geo-distributed Kubernetes clusters
   - Zenoh bridges between regions
   - Edge computing for drone clients

2. **Dynamic Scene Loading**:
   - Load different Unreal maps on demand
   - Switch between environments

3. **Recording & Playback**:
   - Record drone missions (ROS bag files)
   - Replay for analysis
   - Export to video

4. **AI/ML Integration**:
   - Reinforcement learning training
   - Computer vision model inference
   - Swarm intelligence algorithms

5. **Multi-User Missions**:
   - Multiple teams with different drones
   - Collaborative missions
   - Competition mode

6. **Advanced Spectator Features**:
   - Follow specific drone
   - Picture-in-picture (multiple views)
   - Screen recording/streaming to Twitch

7. **Performance Optimizations**:
   - LOD (Level of Detail) for distant drones
   - Culling non-visible objects
   - Reduce network bandwidth (delta compression)

**Time Estimate**: Ongoing, as needed

---

## Technical Decisions & Considerations

### Server Architecture Options

**Option 1: Single Server (Phase 1-10)**
- One Unreal server handles all drones
- Simple architecture
- Scales to ~10-20 drones (hardware dependent)

**Option 2: Multi-Server (Future)**
- Multiple Unreal servers, each simulating subset of drones
- State sync between servers
- More complex but scales to 50+ drones

### Rendering Approaches

**Option 1: Server-Side Rendering (Recommended)**
- Sim server renders all drone cameras
- Clients just receive images
- Requires powerful GPU on server

**Option 2: Client-Side Rendering (Your Original Idea)**
- Each drone client renders its own camera
- Requires running Unreal on each client pod
- Very resource intensive (GPU per pod!)
- More complex Unreal plugin architecture

**Recommendation**: Start with Option 1, consider Option 2 for Phase 12+

### Networking Protocols

1. **Simulation API**: Project AirSim uses NNG (nanomsg-next-generation)
2. **ROS2 Communication**: 
   - Default: FastDDS or CycloneDDS
   - Recommended: Zenoh (better for Kubernetes)
3. **Pixel Streaming**: WebRTC

### GPU Considerations

Kubernetes GPU challenges:
- GPU sharing is limited (mostly 1 pod = 1 GPU)
- NVIDIA GPU Operator required
- Virtual GPU (vGPU) licensing expensive
- MIG (Multi-Instance GPU) for newer cards

Options:
- Multiple physical GPUs per node
- CPU-based rendering (slower)
- Cloud GPU instances (GKE with GPUs, EKS with GPUs)

---

## Development Environment Setup

### Prerequisites
- macOS 13+ (for Phase 1-2)
- Xcode Command Line Tools
- Unreal Engine 5.6 source
- CMake, Make, Python 3.9+
- Docker Desktop (Phase 5+)
- Minikube or Kind (Phase 8+)
- ROS2 Humble (Phase 3+)

### Repository Structure (Recommended)
```
ProjectAirSim/
├── MULTIPLAYER_ROADMAP.md  (this file)
├── docker/
│   ├── server.Dockerfile
│   ├── client.Dockerfile
│   └── minimap.Dockerfile
├── k8s/
│   ├── base/
│   │   ├── sim-server.yaml
│   │   ├── drone-statefulset.yaml
│   │   ├── minimap.yaml
│   │   └── zenoh-router.yaml
│   └── overlays/
│       ├── dev/
│       └── prod/
├── scripts/
│   ├── build-docker-images.sh
│   ├── deploy-k8s.sh
│   └── scale-drones.sh
├── config/
│   ├── scenes/
│   │   └── multi-drone-blocks.jsonc
│   └── robots/
│       └── drone-simple.jsonc
└── minimap/
    ├── backend/  (ROS2 subscriber + WebSocket server)
    └── frontend/  (Three.js visualization)
```

---

## Testing Strategy

### Per-Phase Testing

**Phase 1-2**: Manual testing, visual verification

**Phase 3+**: Automated testing
- Unit tests for ROS2 nodes
- Integration tests for multi-drone scenarios
- Performance benchmarks (FPS, latency)

**Phase 8+**: Kubernetes testing
- Chaos engineering (kill random pods)
- Load testing (increase drone count)
- Network partition testing

### Test Scenarios

1. **Formation Flight**: Drones maintain relative positions
2. **Collision Avoidance**: Drones avoid colliding
3. **Leader-Follower**: One drone follows another
4. **Search Pattern**: Drones cover area systematically
5. **Dynamic Scaling**: Add/remove drones mid-mission

---

## Estimated Total Timeline

| Phase | Description | Duration |
|-------|-------------|----------|
| 1 | Baseline Mac Setup | 1-2 days |
| 2 | Multiple Drones Single Machine | 2-3 days |
| 3 | ROS2 Integration | 3-5 days |
| 4 | Headless Mode | 2-3 days |
| 5 | Docker Single Drone | 4-6 days |
| 6 | Docker Multiple Drones | 3-4 days |
| 7 | Zenoh Integration | 4-5 days |
| 8 | Local Kubernetes | 5-7 days |
| 9 | Pixel Streaming | 5-7 days |
| 10 | Minimap | 4-6 days |
| 11 | Production Hardening | 7-10 days |

**Total Core Development**: 40-58 days (~2-3 months of focused work)

**Phase 12 (Advanced Features)**: Ongoing, project-dependent

---

## Success Metrics

### Technical Metrics
- ✅ Support 10+ simultaneous drones
- ✅ < 50ms ROS2 message latency
- ✅ 30+ FPS camera rendering per drone
- ✅ < 5 second pod startup time
- ✅ 99% uptime over 24 hours
- ✅ Graceful handling of pod failures

### User Experience Metrics
- ✅ Easy one-command deployment (`kubectl apply -f k8s/`)
- ✅ Real-time minimap updates
- ✅ Smooth spectator camera controls
- ✅ < 2 second latency for pixel streaming

---

## Risk Assessment & Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|---------|------------|
| GPU availability in K8s | High | High | Plan CPU fallback, use cloud GPUs |
| ROS2 networking complexity | Medium | Medium | Use Zenoh early, extensive testing |
| Unreal performance in container | Medium | High | Optimize settings, use -RenderOffScreen |
| Kubernetes learning curve | Medium | Medium | Start with docker-compose, good docs |
| Pixel streaming latency | Low | Medium | WebRTC tuning, local testing first |

---

## Resources & References

### Documentation
- [Project AirSim Docs](./docs/)
- [ROS2 Humble Docs](https://docs.ros.org/en/humble/)
- [Kubernetes Docs](https://kubernetes.io/docs/)
- [Zenoh Documentation](https://zenoh.io/docs/)
- [Unreal Pixel Streaming](https://docs.unrealengine.com/5.6/en-US/pixel-streaming-in-unreal-engine/)

### Community
- [Project AirSim Discord](https://discord.gg/XprQ2w64uj)
- [ROS2 Discourse](https://discourse.ros.org/)
- [Kubernetes Slack](https://slack.k8s.io/)

### Tools
- [k9s](https://k9scli.io/) - Kubernetes CLI UI
- [Lens](https://k8slens.dev/) - Kubernetes IDE
- [kubectx/kubens](https://github.com/ahmetb/kubectx) - Context switching

---

## Next Steps

1. ✅ Review this roadmap
2. ⏩ Start Phase 1: Baseline Mac Setup
3. Create GitHub issues for each phase
4. Set up project tracking (e.g., GitHub Projects)
5. Document learnings in a dev journal
6. Share progress with community (Discord)

---

## Notes

- This roadmap is a living document - update as you learn!
- Don't be afraid to skip or reorder phases based on learnings
- Some phases can be parallelized (e.g., Minimap while doing K8s)
- Focus on getting each phase working well before moving on
- It's okay to revisit earlier phases with new knowledge

---

**Document Version**: 1.0  
**Last Updated**: 2024-11-19  
**Author**: AI Assistant + Victor Bjorkgren  
**Project**: Project AirSim Multiplayer Kubernetes Deployment

---

**Copyright**: MIT License  
Based on Project AirSim © 2025 IAMAI CONSULTING CORP

