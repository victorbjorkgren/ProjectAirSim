# Building UE Linux ARM64 on Apple Silicon

## Why ARM64?

Building UE Linux for **ARM64** instead of x86-64 on Apple Silicon Macs provides several advantages:

### ✅ Benefits

1. **No Emulation** - Runs natively on Apple Silicon, avoiding QEMU emulation overhead
2. **No .NET Binary Issues** - ARM64 .NET binaries work perfectly, avoiding the GitDependencies segfault
3. **Faster Builds** - Native execution is significantly faster than emulation
4. **Better Compatibility** - No compatibility issues with pre-built binaries

### ⚠️ Considerations

1. **Target Platform** - The built UE will be for ARM64 Linux, not x86-64
   - This is fine if you're deploying to ARM64 Linux servers (AWS Graviton, etc.)
   - If you need x86-64, you'll need to build on a native x86-64 machine

2. **Docker Runtime** - Your Docker containers will need to run ARM64 Linux
   - Most modern Linux distributions support ARM64
   - Docker images need to be ARM64 compatible

3. **Compatibility** - Some third-party plugins or tools may not have ARM64 Linux versions
   - Check compatibility before deploying

## Architecture

- **Build Platform**: macOS (Apple Silicon - ARM64)
- **Target Platform**: Linux ARM64 (aarch64)
- **Container Platform**: linux/arm64
- **No Emulation**: Native execution throughout

## Usage

The build process is the same, but now builds ARM64:

```bash
cd CloudMultiPlayerProject/docker/build_ue
./build_ue_linux_from_mac.sh
```

The output will be an ARM64 Linux build at:
```
~/projects/DroneProject/UE/LocalBuilds/Engine/Linux/
```

## Deployment

When deploying the built UE:

1. **Docker Server**: Use ARM64 Linux base images
   - `FROM --platform=linux/arm64 ubuntu:22.04`
   - Or use multi-arch images that support ARM64

2. **Kubernetes**: Ensure your cluster has ARM64 nodes
   - AWS EKS supports Graviton instances
   - Or use x86-64 nodes if needed (but then you'd need x86-64 UE)

3. **Cloud Services**: Many cloud providers support ARM64
   - AWS Graviton instances
   - Azure Ampere Altra instances
   - Google Cloud Tau T2A instances

## Switching Back to x86-64

If you need x86-64 builds instead, you can:
1. Change `--platform=linux/arm64` to `--platform=linux/amd64` in the Dockerfile
2. Change `WithLinuxAArch64=true` to `WithLinuxAArch64=false` in build_ue_linux.sh
3. Change `WithLinuxX86_64=false` to `WithLinuxX86_64=true`
4. Be aware of the .NET emulation issues documented in KNOWN_ISSUES.md


