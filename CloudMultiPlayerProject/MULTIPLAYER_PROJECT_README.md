# Project AirSim Multiplayer Kubernetes Project

## 🎯 Vision

Transform Project AirSim's Blocks environment into a **distributed, cloud-native multiplayer drone simulation** where:

- 🚁 **Multiple drones** run as independent Kubernetes pods
- 📹 **Each client** handles its own camera rendering
- 🎮 **Central server** maintains overall simulation state
- 🔌 **ROS2 nodes** communicate via Zenoh DDS middleware
- ☁️ **Cloud-native** design for Kubernetes deployment
- 👁️ **Spectator mode** with pixel streaming and minimap
- 🎯 **Headless operation** for efficient cloud/edge computing

## 📚 Documentation Structure

This project consists of several interconnected documents:

### 1. 🗺️ **MULTIPLAYER_ROADMAP.md** ← Start Here!
The **complete technical roadmap** with 12 incremental phases from local Mac setup to production Kubernetes deployment.

**What's inside:**
- Detailed architecture design
- Phase-by-phase breakdown (40-58 days estimated)
- Technical decisions and rationale
- Docker/Kubernetes configurations
- Risk assessment
- Success criteria for each phase

**Read this to understand the full journey.**

### 2. 🚀 **PHASE1_QUICKSTART.md**
**Quick start guide** to get you running on macOS within hours.

**What's inside:**
- Step-by-step setup instructions
- Prerequisites checklist
- Example Python script
- Common issues & solutions
- Verification steps

**Read this to start coding immediately.**

### 3. ✅ **PROJECT_STATUS.md**
**Progress tracking checklist** for all 12 phases.

**What's inside:**
- Checkbox lists for each phase
- Notes sections for issues/solutions
- Metrics dashboard
- Architecture decisions log
- Quick command reference

**Update this as you progress through the project.**

### 4. 📖 **This Document**
High-level overview and navigation guide.

---

## 🏗️ Project Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                  Kubernetes Cluster (Cloud/Edge)             │
│                                                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ Drone Pod 1  │  │ Drone Pod 2  │  │ Drone Pod N  │     │
│  │ ├─ ROS2 Node │  │ ├─ ROS2 Node │  │ ├─ ROS2 Node │     │
│  │ ├─ Camera    │  │ ├─ Camera    │  │ ├─ Camera    │     │
│  │ └─ Control   │  │ └─ Control   │  │ └─ Control   │     │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘     │
│         │                 │                 │              │
│         └─────────────────┴─────────────────┘              │
│                           │                                 │
│                    Zenoh DDS Router                         │
│                           │                                 │
│         ┌─────────────────┼─────────────────┐              │
│         │                 │                 │              │
│  ┌──────▼──────────┐  ┌──▼────────┐  ┌────▼──────┐       │
│  │  Sim Server     │  │ Spectator │  │  Minimap  │       │
│  │  (Unreal)       │  │ (Pixel    │  │  (Web UI) │       │
│  │  - Physics      │  │  Stream)  │  │           │       │
│  │  - Collisions   │  │           │  │           │       │
│  │  - State Mgmt   │  │           │  │           │       │
│  └─────────────────┘  └───────────┘  └───────────┘       │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## 🛤️ The Journey: 12 Phases

| Phase | Name | Duration | Status |
|-------|------|----------|--------|
| **1** | Baseline Mac Setup | 1-2 days | ⏩ **START HERE** |
| **2** | Multiple Drones (Single Machine) | 2-3 days | 📋 Planned |
| **3** | ROS2 Integration | 3-5 days | 📋 Planned |
| **4** | Headless Mode | 2-3 days | 📋 Planned |
| **5** | Docker Single Drone | 4-6 days | 📋 Planned |
| **6** | Docker Multiple Drones | 3-4 days | 📋 Planned |
| **7** | Zenoh Integration | 4-5 days | 📋 Planned |
| **8** | Local Kubernetes | 5-7 days | 📋 Planned |
| **9** | Pixel Streaming Spectator | 5-7 days | 📋 Planned |
| **10** | Minimap Visualization | 4-6 days | 📋 Planned |
| **11** | Production Hardening | 7-10 days | 📋 Planned |
| **12** | Advanced Features | Ongoing | 📋 Planned |

**Total Estimated Time**: 40-58 days of focused development

---

## 🚀 Quick Start (5 Minutes to First Flight)

### Prerequisites
- macOS 13+
- Xcode Command Line Tools
- CMake 3.15+
- Python 3.9+
- Unreal Engine 5.6

### Three-Step Launch

```bash
# 1. Build Project AirSim
cd /Users/victorbjorkgren/projects/DroneProject/ProjectAirSim
./build_macos.sh

# 2. Open Blocks in Unreal Editor
cd unreal/Blocks
open Blocks.uproject
# Press Play in the editor

# 3. Run Python client (in new terminal)
cd client/python
python3 -m venv venv
source venv/bin/activate
pip install -e .
python example_user_scripts/hello_drone.py
```

**See `PHASE1_QUICKSTART.md` for detailed instructions.**

---

## 📁 Project Structure (New Files)

```
ProjectAirSim/
├── MULTIPLAYER_PROJECT_README.md  ← You are here
├── MULTIPLAYER_ROADMAP.md         ← Detailed technical roadmap
├── PHASE1_QUICKSTART.md           ← Quick start guide
├── PROJECT_STATUS.md              ← Progress tracker
│
├── docker/                        ← (To be created in Phase 5)
│   ├── server.Dockerfile
│   ├── client.Dockerfile
│   └── minimap.Dockerfile
│
├── k8s/                          ← (To be created in Phase 8)
│   ├── base/
│   │   ├── sim-server.yaml
│   │   ├── drone-statefulset.yaml
│   │   └── minimap.yaml
│   └── overlays/
│       ├── dev/
│       └── prod/
│
├── scripts/                      ← (To be created)
│   ├── build-docker-images.sh
│   ├── deploy-k8s.sh
│   └── scale-drones.sh
│
├── config/                       ← (To be created in Phase 2)
│   ├── scenes/
│   │   └── multi-drone-blocks.jsonc
│   └── robots/
│       └── drone-simple.jsonc
│
└── minimap/                      ← (To be created in Phase 10)
    ├── backend/
    └── frontend/
```

---

## 🎯 Key Features

### ✅ What Works Now (Phase 1)
- Single drone simulation in Blocks environment
- Python client control via API
- Camera image capture
- ROS2 bridge (basic)

### 🚧 What We're Building

#### Phase 2-4: Local Multi-Drone
- [x] Multiple independent drones
- [x] ROS2 communication between drones
- [x] Headless operation

#### Phase 5-7: Containerization
- [x] Docker images for server & clients
- [x] Multi-container deployment
- [x] Zenoh for efficient DDS communication

#### Phase 8-10: Kubernetes & Visualization
- [x] Full Kubernetes deployment
- [x] Pixel streaming spectator mode
- [x] Real-time minimap with drone positions

#### Phase 11-12: Production & Beyond
- [x] Monitoring & logging
- [x] Auto-scaling
- [x] Advanced features (AI, multi-region, etc.)

---

## 🛠️ Technology Stack

| Layer | Technology | Purpose |
|-------|-----------|---------|
| **Simulation** | Unreal Engine 5.6 | Physics, rendering, environment |
| **Drone Framework** | Project AirSim | Drone simulation framework |
| **Communication** | ROS2 Humble/Iron | Robotics middleware |
| **DDS** | Zenoh | Efficient, cloud-native messaging |
| **Containerization** | Docker | Application packaging |
| **Orchestration** | Kubernetes | Container orchestration |
| **Streaming** | Pixel Streaming (WebRTC) | Remote visualization |
| **Visualization** | Three.js / RViz2 | Minimap & monitoring |
| **Monitoring** | Prometheus + Grafana | Metrics & dashboards |
| **Logging** | ELK or Loki | Centralized logging |

---

## 📊 Success Metrics

By the end of this project, we aim to achieve:

| Metric | Target | Current |
|--------|--------|---------|
| Max Simultaneous Drones | 10+ | 1 |
| ROS2 Message Latency | < 50ms | TBD |
| Camera FPS (per drone) | 30+ | TBD |
| Pod Startup Time | < 5s | TBD |
| System Uptime | 99%+ | TBD |
| Pixel Stream Latency | < 100ms | TBD |

---

## 🤔 Why This Architecture?

### Problem: Scaling Drone Simulations
- Traditional: Single machine, limited by CPU/GPU
- Rendering all cameras on one server = bottleneck
- Hard to distribute workload

### Solution: Cloud-Native Microservices
- **Server**: Manages physics, collisions, state
- **Clients**: Each drone as independent pod
- **Scalable**: Add drones by increasing replicas
- **Resilient**: Pod failures auto-recover
- **Flexible**: Run on local cluster or cloud (GKE, EKS, AKS)

### Why Kubernetes?
- Industry standard for container orchestration
- Auto-scaling, self-healing, load balancing
- Easy deployment, updates, rollbacks
- Portable across cloud providers

### Why ROS2 + Zenoh?
- **ROS2**: De facto standard for robotics
- **Zenoh**: Modern, efficient DDS replacement
- Better performance in Kubernetes networking
- Lower latency, less overhead than traditional DDS

---

## 🎓 Learning Outcomes

By completing this project, you'll gain expertise in:

1. **Unreal Engine**: Headless operation, pixel streaming, multi-actor scenes
2. **ROS2**: Multi-node systems, custom messages, launch files
3. **Docker**: Multi-stage builds, optimization, networking
4. **Kubernetes**: Deployments, StatefulSets, Services, ConfigMaps
5. **Zenoh**: Modern DDS, cloud-native messaging
6. **Distributed Systems**: State management, networking, scalability
7. **DevOps**: CI/CD, monitoring, logging, security

---

## 🔍 Use Cases

### Research
- Multi-agent reinforcement learning
- Swarm intelligence algorithms
- Collaborative path planning

### Education
- Teach distributed systems
- Robotics courses
- Cloud-native application design

### Industry
- Drone fleet simulation
- Warehouse automation testing
- Search & rescue scenario training

### Competition
- Multi-drone racing leagues
- CTF (Capture the Flag) with drones
- Formation flying contests

---

## 📚 Recommended Reading Order

1. **Start Here**: `MULTIPLAYER_PROJECT_README.md` (this file)
2. **Understand the Plan**: `MULTIPLAYER_ROADMAP.md`
3. **Get Hands-On**: `PHASE1_QUICKSTART.md`
4. **Track Progress**: `PROJECT_STATUS.md` (update as you go!)

### During Development
- Reference `MULTIPLAYER_ROADMAP.md` for detailed phase instructions
- Update `PROJECT_STATUS.md` after completing each task
- Create issues/notes for blockers
- Document architecture decisions in `PROJECT_STATUS.md`

---

## 🤝 Contributing & Support

### Questions?
- **Discord**: [Project AirSim Community](https://discord.gg/XprQ2w64uj)
- **GitHub Issues**: [Report bugs/features](https://github.com/iamaisim/ProjectAirSim/issues)
- **Docs**: Check `docs/faq.md`

### Share Your Progress!
We'd love to hear about your journey:
- Post updates in Discord
- Share screenshots/videos
- Contribute back improvements

---

## 🎬 Next Steps

### Right Now (5 minutes)
1. ✅ You've read this overview
2. ⏩ Open `PHASE1_QUICKSTART.md`
3. 🚀 Start building!

### Today (2-4 hours)
- Complete Phase 1 setup
- Get first drone flying
- Capture your first camera image

### This Week
- Complete Phase 1 verification
- Start Phase 2: Multiple drones
- Update `PROJECT_STATUS.md`

### This Month
- Complete Phases 1-4 (local setup)
- Start containerization (Phase 5)
- Begin learning Kubernetes basics

### This Quarter
- Complete Phases 1-8 (Kubernetes deployment)
- Have basic multiplayer system running
- Start adding visualization (Phases 9-10)

---

## 🏆 Milestones

- [ ] **Milestone 1**: Single drone flying on Mac
- [ ] **Milestone 2**: 3 drones flying simultaneously
- [ ] **Milestone 3**: ROS2 control working
- [ ] **Milestone 4**: Docker deployment working
- [ ] **Milestone 5**: Kubernetes cluster running
- [ ] **Milestone 6**: Spectator mode + minimap
- [ ] **Milestone 7**: Production-ready system

---

## ⚠️ Important Notes

### This is a Learning Journey
- Don't rush through phases
- Experiment and break things
- Document what you learn
- Some phases will take longer than estimated

### Adjust as Needed
- This roadmap is a guide, not a rulebook
- Skip phases that don't apply
- Reorder based on your priorities
- Add custom phases for your specific needs

### Resource Requirements
- **Phase 1-4**: Your Mac (no additional hardware)
- **Phase 5-7**: Docker (local machine OK)
- **Phase 8+**: Kubernetes cluster (can use minikube/kind locally)
- **Production**: Cloud resources (GCP, AWS, Azure) or powerful local hardware

---

## 📞 Project Contacts

**Project Lead**: Victor Bjorkgren

**Maintainer**: Project AirSim Community

**Commercial Support**: [IAMAI Simulations](https://www.iamaisim.com)

---

## 📝 License

MIT License

Based on Project AirSim  
Copyright (C) Microsoft Corporation  
Copyright (C) 2025 IAMAI CONSULTING CORP

---

## 🌟 Final Thoughts

This is an ambitious project that combines:
- 🎮 Game engine technology (Unreal)
- 🤖 Robotics middleware (ROS2)
- ☁️ Cloud-native infrastructure (Kubernetes)
- 🚁 Autonomous systems (drones)

**You're building something awesome!**

Take it one phase at a time, enjoy the learning process, and don't hesitate to reach out for help.

**Ready to start? Open `PHASE1_QUICKSTART.md` and let's fly! 🚁**

---

**Document Version**: 1.0  
**Last Updated**: 2024-11-19  
**Project Status**: Phase 1 - Ready to Begin ✨

