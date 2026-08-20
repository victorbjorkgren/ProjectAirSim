# Known Issues with Building UE Linux from macOS

## GitDependencies Segfault Under QEMU Emulation

**Problem**: The `GitDependencies` .NET binary segfaults when running in an x86-64 Linux container on Apple Silicon Macs.

**Error**: 
```
Segmentation fault ./Engine/Binaries/DotNET/GitDependencies/linux-x64/GitDependencies
Result: 139
```

**Root Cause**: 
- .NET binaries have compatibility issues when running under QEMU x86-64 emulation
- The pre-built GitDependencies tool in UE source is a .NET binary
- This is a fundamental limitation of emulation, not a bug in our setup

**Workarounds**:

### Option 1: Use a Native Linux Machine (Recommended)
Build on a native Linux x86-64 machine or VM:
- Physical Linux machine
- Linux VM (VMware, Parallels, VirtualBox)
- Cloud Linux instance (AWS EC2, Azure, GCP)

### Option 2: Use a Linux VM Instead of Docker
Docker's QEMU emulation has issues with .NET binaries. A full Linux VM may work better:
- VMware Fusion (better emulation than Docker)
- Parallels Desktop
- UTM (free, open-source)

### Option 3: Manual Dependency Download
If you can get the dependencies downloaded another way, you can skip Setup.sh:
1. Download dependencies on a working Linux machine
2. Copy `Engine/Extras/ThirdPartyNotUE/` to your source
3. Skip Setup.sh and proceed directly to build

### Option 4: Build on Cloud CI/CD
Use a cloud-based CI/CD service that provides native Linux x86-64 runners:
- GitHub Actions (ubuntu-latest)
- GitLab CI
- Azure DevOps
- CircleCI

## Why This Happens

Docker Desktop on Apple Silicon uses QEMU to emulate x86-64 Linux containers. While this works for most software, .NET binaries have specific requirements that don't translate well through emulation:

1. .NET runtime uses JIT compilation that interacts closely with CPU features
2. Memory management and garbage collection have platform-specific optimizations
3. The GitDependencies binary may use platform-specific .NET features

## Alternative: Pre-built UE Linux

Instead of building from source, consider:
- Using Epic's pre-built Linux binaries (if available for your UE version)
- Using a pre-built Docker image with UE Linux already installed
- Building on a cloud service and downloading the result

## Status

This is a known limitation of the Docker-based approach on Apple Silicon. The build system will attempt retries, but if GitDependencies continues to fail, you'll need to use one of the alternatives above.

