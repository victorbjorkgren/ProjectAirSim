# ProjectAirSim macOS Build Guide

This guide will help you build ProjectAirSim on macOS.

## Prerequisites

### Required Software
- **Xcode Command Line Tools**: Required for C++ compilation
- **Homebrew**: Package manager for macOS (recommended)
- **CMake**: Build system generator
- **Ninja**: Fast build system
- **LLVM/Clang**: C++ compiler (recommended over system clang)

### System Requirements
- macOS 10.15 (Catalina) or later
- At least 8GB RAM (16GB recommended)
- At least 10GB free disk space

## Quick Start

1. **Run the setup script** (this will install all prerequisites):
   ```bash
   ./setup_macos_dev_tools.sh
   ```

2. **Restart your terminal** or run:
   ```bash
   source ~/.zshrc
   ```

3. **Build the project**:
   ```bash
   ./build_macos.sh all_no_test
   ```

## Manual Setup

If you prefer to install dependencies manually:

### Install Homebrew
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### Install Build Tools
```bash
brew install cmake ninja llvm rsync
```

### Install Xcode Command Line Tools
```bash
xcode-select --install
```

### Set Environment Variables
Add these to your `~/.zshrc` file:

For Apple Silicon (M1/M2):
```bash
export CC=/opt/homebrew/opt/llvm/bin/clang
export CXX=/opt/homebrew/opt/llvm/bin/clang++
```

For Intel Mac:
```bash
export CC=/usr/local/opt/llvm/bin/clang
export CXX=/usr/local/opt/llvm/bin/clang++
```

## Build Targets

The following build targets are available:

- **`all`**: Build + Test + Package everything
- **`all_no_test`**: Build + Package everything (skip tests)
- **`rebuild_all`**: Clean + Build + Test + Package everything
- **`clean`**: Clean build artifacts
- **`simlibs_debug`**: Build simulation libraries for Debug
- **`simlibs_release`**: Build simulation libraries for Release
- **`test_simlibs_debug`**: Test simulation libraries for Debug
- **`test_simlibs_release`**: Test simulation libraries for Release

## Unreal Engine Integration

To build Unreal Engine components, you need to:

1. **Set the UE_ROOT environment variable**:
   ```bash
   export UE_ROOT=/path/to/your/UnrealEngine
   ```

2. **Build with Unreal Engine targets**:
   ```bash
   ./build_macos.sh blocks_development
   ```

## Python Development

For Python development, install the required dependencies:

```bash
cd client/python/projectairsim
pip install -r requirements.txt
```

## Troubleshooting

### Common Issues

1. **"clang not found" error**:
   - Make sure Xcode Command Line Tools are installed
   - Check that environment variables are set correctly

2. **"cmake not found" error**:
   - Install CMake via Homebrew: `brew install cmake`

3. **"ninja not found" error**:
   - Install Ninja via Homebrew: `brew install ninja`

4. **Build failures with system clang**:
   - Use Homebrew LLVM instead: `brew install llvm`
   - Set environment variables as shown above

5. **Permission errors**:
   - Make sure build scripts are executable: `chmod +x build_macos.sh`

### Getting Help

- Check the main [README.md](README.md) for general project information
- Review the [docs/](docs/) directory for detailed documentation
- Check the [CMakeLists.txt](CMakeLists.txt) for build configuration details

## Architecture Support

- **Apple Silicon (M1/M2)**: Fully supported
- **Intel Mac**: Fully supported
- **Universal Binary**: Not currently supported (builds for native architecture only)

## Performance Notes

- Building on Apple Silicon Macs is significantly faster than Intel Macs
- Use `-j` flag with make for parallel builds (e.g., `make -j8`)
- Consider using an external SSD for better I/O performance during builds
