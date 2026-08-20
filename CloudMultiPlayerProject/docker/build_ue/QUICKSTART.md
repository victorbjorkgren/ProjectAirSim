# Quick Start: Build UE Linux from macOS

## One-Command Build

```bash
cd CloudMultiPlayerProject/docker/build_ue
./build_ue_linux_from_mac.sh
```

That's it! The script will:
1. ✅ Check prerequisites (Docker, disk space, UE source)
2. ✅ Build the Docker image (first time only)
3. ✅ Run the UE Linux build (4+ hours)
4. ✅ Output to `~/projects/DroneProject/UE_Linux_Build/`

## Prerequisites Check

Before running, ensure:
- ✅ Docker Desktop is running
- ✅ UE source is at `~/projects/DroneProject/UE/`
- ✅ At least 250GB free disk space

## Monitor Progress

In another terminal:
```bash
# View logs
docker logs -f $(docker ps -q --filter ancestor=ue-linux-builder:5.7)

# Check disk usage
docker system df
df -h ~/projects/DroneProject/
```

## After Build Completes

```bash
# Set UE_ROOT
export UE_ROOT=~/projects/DroneProject/UE_Linux_Build

# Verify it works
ls $UE_ROOT/Engine/Build/BatchFiles/Linux/Build.sh

# Build Project AirSim Linux
cd CloudMultiPlayerProject/docker/build
./build_linux_in_docker.sh
```

## Troubleshooting

**Build fails?** Check `docker logs` for errors.

**Out of space?** Free up 250GB+ or use external drive:
```bash
export UE_OUTPUT=/Volumes/ExternalDrive/UE_Linux_Build
./build_ue_linux_from_mac.sh
```

**Taking too long?** Normal build time is 4+ hours. Be patient!

For more details, see [README.md](README.md).


