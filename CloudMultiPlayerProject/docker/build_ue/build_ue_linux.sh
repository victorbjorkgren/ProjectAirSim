#!/bin/bash
# Script to build Unreal Engine Linux from source inside Docker
# This script is run inside the container

# Don't exit on error immediately - we need to handle GitDependencies failures
set +e

UE_SOURCE_DIR="${UE_SOURCE_DIR:-/ue-source}"
UE_OUTPUT_DIR="${UE_OUTPUT_DIR:-/ue-output}"

echo "======================================================================="
echo "Unreal Engine Linux Build"
echo "======================================================================="
echo "Source: $UE_SOURCE_DIR"
echo "Output: $UE_OUTPUT_DIR"
echo ""

# Check if source directory exists
if [ ! -d "$UE_SOURCE_DIR" ]; then
    echo "ERROR: UE source directory not found: $UE_SOURCE_DIR"
    echo "Make sure to mount the UE source with: -v /path/to/ue:/ue-source"
    exit 1
fi

# Check if Setup.sh exists
if [ ! -f "$UE_SOURCE_DIR/Setup.sh" ]; then
    echo "ERROR: Setup.sh not found in $UE_SOURCE_DIR"
    echo "This doesn't appear to be a valid UE source directory"
    exit 1
fi

# Create output directory
mkdir -p "$UE_OUTPUT_DIR"

# Build directly from the mounted source directory
# UE can build multiple targets from the same source, and build artifacts
# (LocalBuilds/, Engine/Binaries/, Engine/Intermediate/) will be created in the source tree
echo "======================================================================="
echo "Building directly from mounted source"
echo "======================================================================="
echo "Source: $UE_SOURCE_DIR"
echo "Build artifacts will be created in the source directory"
echo "Final output will be copied to: $UE_OUTPUT_DIR"
echo ""
cd "$UE_SOURCE_DIR"

# Build GitDependencies for ARM64 Linux before running Setup.sh
# The pre-built binary is x86-64 only, so we need to build it for ARM64
echo "======================================================================="
echo "Building GitDependencies for ARM64 Linux"
echo "======================================================================="
GITDEPS_SOURCE="Engine/Source/Programs/GitDependencies"
GITDEPS_OUTPUT="Engine/Binaries/DotNET/GitDependencies/linux-arm64"
GITDEPS_SCRIPT="Engine/Build/BatchFiles/Linux/GitDependencies.sh"

if [ -d "$GITDEPS_SOURCE" ]; then
    echo "Found GitDependencies source, building for linux-arm64..."
    mkdir -p "$GITDEPS_OUTPUT"
    
    cd "$GITDEPS_SOURCE"
    
    # Check if it's a .NET project
    if [ -f "GitDependencies.csproj" ] || [ -f "*.csproj" ]; then
        echo "Building GitDependencies with .NET..."
        # Find the .csproj file
        CSPROJ=$(find . -name "*.csproj" | head -1)
        if [ -n "$CSPROJ" ]; then
            dotnet publish "$CSPROJ" -c Release -r linux-arm64 --self-contained -o "../../../$GITDEPS_OUTPUT" || {
                echo "ERROR: Failed to build GitDependencies for ARM64"
                echo "Trying alternative build method..."
                # Try without self-contained
                dotnet publish "$CSPROJ" -c Release -r linux-arm64 -o "../../../$GITDEPS_OUTPUT" || {
                    echo "ERROR: All build methods failed"
                    exit 1
                }
            }
            
            # Ensure the binary is executable
            if [ -f "../../../$GITDEPS_OUTPUT/GitDependencies" ]; then
                chmod +x "../../../$GITDEPS_OUTPUT/GitDependencies"
                echo "✓ GitDependencies built successfully for ARM64"
                echo "  Location: $UE_SOURCE_DIR/$GITDEPS_OUTPUT/GitDependencies"
            else
                echo "ERROR: GitDependencies binary not found after build"
                exit 1
            fi
        else
            echo "ERROR: No .csproj file found in GitDependencies source"
            exit 1
        fi
    else
        echo "WARNING: GitDependencies source doesn't appear to be a .NET project"
        echo "Checking for alternative build methods..."
    fi
    
    cd "$UE_SOURCE_DIR"
    
    # Create symlink from linux-x64 to linux-arm64 so Setup.sh can use the x64 path
    # but actually get the ARM64 binary. This is more reliable than patching the script.
    if [ -f "$GITDEPS_OUTPUT/GitDependencies" ]; then
        GITDEPS_X64_DIR="Engine/Binaries/DotNET/GitDependencies/linux-x64"
        GITDEPS_ARM64_DIR="Engine/Binaries/DotNET/GitDependencies/linux-arm64"
        
        # Remove existing x64 directory if it exists (might be a broken symlink or old binary)
        if [ -e "$GITDEPS_X64_DIR" ]; then
            echo "Removing existing linux-x64 directory/symlink..."
            rm -rf "$GITDEPS_X64_DIR"
        fi
        
        # Create parent directory if it doesn't exist
        mkdir -p "$(dirname "$GITDEPS_X64_DIR")"
        
        # Create symlink from linux-x64 to linux-arm64
        # Use relative path so symlink works regardless of where UE source is mounted
        echo "Creating symlink: $GITDEPS_X64_DIR -> linux-arm64"
        cd "$(dirname "$GITDEPS_X64_DIR")"
        ln -s "linux-arm64" "$(basename "$GITDEPS_X64_DIR")"
        cd "$UE_SOURCE_DIR"
        
        # Verify the symlink works
        if [ -L "$GITDEPS_X64_DIR" ] && [ -f "$GITDEPS_X64_DIR/GitDependencies" ]; then
            SYMLINK_TARGET=$(readlink -f "$GITDEPS_X64_DIR")
            echo "✓ Symlink created successfully"
            echo "  Symlink: $GITDEPS_X64_DIR"
            echo "  Target: $SYMLINK_TARGET"
            echo "  Binary accessible: Yes"
            echo "  Setup.sh will use ARM64 binary via x64 path"
        else
            echo "ERROR: Symlink verification failed"
            echo "  Symlink exists: $([ -L "$GITDEPS_X64_DIR" ] && echo "Yes" || echo "No")"
            echo "  Binary accessible: $([ -f "$GITDEPS_X64_DIR/GitDependencies" ] && echo "Yes" || echo "No")"
            exit 1
        fi
        
        # Also patch GitDependencies.sh as a backup (in case it's called directly)
        if [ -f "$GITDEPS_SCRIPT" ]; then
            echo "Patching GitDependencies.sh to use ARM64 binary..."
            sed -i 's|linux-x64|linux-arm64|g' "$GITDEPS_SCRIPT"
            echo "✓ GitDependencies.sh patched as backup"
        fi
    else
        echo "WARNING: GitDependencies ARM64 binary not found at $GITDEPS_OUTPUT/GitDependencies"
        echo "Setup.sh will try to use the x86-64 binary, which will fail on ARM64"
    fi
else
    echo "WARNING: GitDependencies source not found at $GITDEPS_SOURCE"
    echo "Setup.sh will try to use the x86-64 binary, which will fail on ARM64"
fi
echo ""

# Verify GitDependencies setup before running Setup.sh
echo "======================================================================="
echo "Verifying GitDependencies setup"
echo "======================================================================="
GITDEPS_X64_DIR="Engine/Binaries/DotNET/GitDependencies/linux-x64"
GITDEPS_ARM64_DIR="Engine/Binaries/DotNET/GitDependencies/linux-arm64"

if [ -L "$GITDEPS_X64_DIR" ]; then
    echo "✓ Symlink exists: $GITDEPS_X64_DIR"
    echo "  Target: $(readlink "$GITDEPS_X64_DIR")"
    if [ -f "$GITDEPS_X64_DIR/GitDependencies" ]; then
        echo "✓ GitDependencies binary accessible via symlink"
        # Check if it's actually ARM64
        if file "$GITDEPS_X64_DIR/GitDependencies" | grep -q "ARM\|aarch64"; then
            echo "✓ Binary is ARM64 (correct)"
        else
            echo "⚠ WARNING: Binary architecture check failed, but continuing..."
        fi
    else
        echo "✗ ERROR: GitDependencies binary not accessible via symlink!"
        echo "  Expected at: $GITDEPS_X64_DIR/GitDependencies"
        exit 1
    fi
elif [ -d "$GITDEPS_X64_DIR" ]; then
    echo "⚠ WARNING: $GITDEPS_X64_DIR exists but is not a symlink (might be x86-64 binary)"
    echo "  This will cause Rosetta errors. Removing and recreating symlink..."
    rm -rf "$GITDEPS_X64_DIR"
    if [ -d "$GITDEPS_ARM64_DIR" ]; then
        ln -s "$(basename "$GITDEPS_ARM64_DIR")" "$GITDEPS_X64_DIR"
        echo "✓ Symlink recreated"
    else
        echo "✗ ERROR: Cannot create symlink - ARM64 directory not found"
        exit 1
    fi
else
    echo "⚠ WARNING: No GitDependencies setup found"
    echo "  Setup.sh will try to download/use x86-64 binary, which will fail"
    echo "  Continuing anyway - Setup.sh might handle this..."
fi
echo ""

# Step 1: Run Setup.sh
echo ""
echo "======================================================================="
echo "Step 1: Running Setup.sh"
echo "======================================================================="
echo "This downloads dependencies and may take 10-30 minutes..."
echo "Note: Setup.sh is idempotent - if run again, it will skip already downloaded files"
echo ""

# Try to run Setup.sh, but if GitDependencies fails, we'll try a workaround
if ! ./Setup.sh 2>&1 | tee /tmp/setup.log; then
    SETUP_EXIT_CODE=${PIPESTATUS[0]}
    
    # Check if the failure is due to GitDependencies issues (segfault, Rosetta error, etc.)
    if grep -q "Segmentation fault.*GitDependencies" /tmp/setup.log || \
       grep -q "Result: 139" /tmp/setup.log || \
       grep -q "rosetta error.*GitDependencies" /tmp/setup.log || \
       grep -q "Result: 133" /tmp/setup.log || \
       grep -q "Trace/breakpoint trap.*GitDependencies" /tmp/setup.log; then
        echo ""
        echo "======================================================================="
        echo "GitDependencies is failing (segfault/Rosetta error)"
        echo "======================================================================="
        echo "This indicates the x86-64 GitDependencies binary is being used instead of ARM64."
        echo "The symlink workaround may not have worked, or Setup.sh is bypassing it."
        echo ""
        echo "Attempting workaround: Running Setup.sh with retry logic..."
        echo ""
        
        # Try running Setup.sh multiple times - sometimes it works on retry
        # Or try to manually download dependencies
        MAX_RETRIES=3
        RETRY_COUNT=0
        SETUP_SUCCESS=false
        
        while [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
            RETRY_COUNT=$((RETRY_COUNT + 1))
            echo "Retry attempt $RETRY_COUNT of $MAX_RETRIES..."
            
            # Try with a small delay
            sleep 2
            
            if timeout 300 ./Setup.sh 2>&1 | tee /tmp/setup_retry.log; then
                SETUP_SUCCESS=true
                break
            fi
            
            # Check if it's still the same error
            if ! grep -q "Segmentation fault.*GitDependencies" /tmp/setup_retry.log && \
               ! grep -q "Result: 139" /tmp/setup_retry.log && \
               ! grep -q "rosetta error.*GitDependencies" /tmp/setup_retry.log && \
               ! grep -q "Result: 133" /tmp/setup_retry.log && \
               ! grep -q "Trace/breakpoint trap.*GitDependencies" /tmp/setup_retry.log; then
                # Different error, don't retry
                break
            fi
        done
        
            if [ "$SETUP_SUCCESS" = false ]; then
            echo ""
            echo "======================================================================="
            echo "Setup.sh failed after $MAX_RETRIES retries"
            echo "======================================================================="
            echo ""
            echo "The GitDependencies tool is not working - it's trying to use x86-64 binary on ARM64."
            echo "This should not happen if the symlink was created correctly."
            echo ""
            echo "Debugging steps:"
            echo "  1. Check if symlink exists: ls -la Engine/Binaries/DotNET/GitDependencies/"
            echo "  2. Check if ARM64 binary exists: ls -la Engine/Binaries/DotNET/GitDependencies/linux-arm64/"
            echo "  3. Verify symlink target: readlink Engine/Binaries/DotNET/GitDependencies/linux-x64"
            echo ""
            echo "See KNOWN_ISSUES.md for detailed information and alternatives."
            echo ""
            echo "Quick alternatives:"
            echo "  1. Build UE on a native Linux x86-64 machine (recommended)"
            echo "  2. Use a Linux VM instead of Docker (VMware/Parallels)"
            echo "  3. Build on a cloud Linux instance (AWS EC2, etc.)"
            echo "  4. Use GitHub Actions or similar CI/CD with native Linux runners"
            echo ""
            echo "Checking if any dependencies were downloaded..."
            if [ -d "Engine/Extras/ThirdPartyNotUE" ]; then
                echo "  ✓ Some dependencies directory exists - partial download may have occurred"
                echo "  You may be able to continue, but the build will likely fail later."
                echo ""
                read -p "Continue anyway? (y/N) " -n 1 -r
                echo
                if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                    exit 1
                fi
                echo "Proceeding with partial dependencies - build may fail..."
            else
                echo "  ✗ No dependencies downloaded"
                echo ""
                echo "Cannot proceed without dependencies. Please use one of the alternatives above."
                exit 1
            fi
        fi
    else
        echo ""
        echo "ERROR: Setup.sh failed with a different error!"
        echo "Exit code: $SETUP_EXIT_CODE"
        echo "Check the logs above for details."
        exit 1
    fi
fi

# Re-enable exit on error for the rest of the script
set -e

# Step 2: Remove unnecessary platforms (optional, saves space)
echo ""
echo "======================================================================="
echo "Step 2: Cleaning up unnecessary platforms"
echo "======================================================================="
# Remove platforms we don't need (keep Linux, remove others to save space)
if [ -d "Engine/Platforms" ]; then
    echo "Removing unnecessary platform folders..."
    find Engine/Platforms -mindepth 1 -maxdepth 1 -type d ! -name "Linux" -exec rm -rf {} + 2>/dev/null || true
    echo "Platform cleanup complete"
fi

# Step 3: Build the installed build
echo ""
echo "======================================================================="
echo "Step 3: Building Unreal Engine Linux ARM64 (Installed Build)"
echo "======================================================================="
echo "Architecture: ARM64 (aarch64) - native build on Apple Silicon"
echo "WARNING: This will take 4+ hours and require ~200GB disk space!"
echo "The build is running in the background. Check progress with:"
echo "  docker logs -f <container-id>"
echo ""
echo "Starting build at: $(date)"
echo ""

# Run the build for ARM64 Linux
# Building for ARM64 (aarch64) which runs natively on Apple Silicon
./Engine/Build/BatchFiles/RunUAT.sh BuildGraph \
    -target="Make Installed Build Linux" \
    -script=Engine/Build/InstalledEngineBuild.xml \
    -set:HostPlatformOnly=true \
    -set:WithLinuxAArch64=true \
    -set:WithLinuxX86_64=false \
    -set:WithFullDebugInfo=false \
    -set:WithDDC=true \
    -set:GameConfigurations="DebugGame;Development" \
    || {
    echo ""
    echo "======================================================================="
    echo "ERROR: Build failed!"
    echo "======================================================================="
    echo "Check the logs above for errors."
    exit 1
}

echo ""
echo "======================================================================="
echo "Build completed successfully!"
echo "======================================================================="
echo "Completed at: $(date)"
echo ""

# Step 4: Report build location (copy to output directory is optional)
# For ARM64 builds, the output is in LocalBuilds/Engine/Linux (same path as x86-64)
if [ -d "LocalBuilds/Engine/Linux" ]; then
    INSTALLED_BUILD_PATH="$UE_SOURCE_DIR/LocalBuilds/Engine/Linux"
    
    echo "======================================================================="
    echo "SUCCESS: Unreal Engine Linux build complete!"
    echo "======================================================================="
    echo "Installed build location: $INSTALLED_BUILD_PATH"
    echo ""
    
    # Optionally copy to output directory if it's different from source location
    if [ -n "$UE_OUTPUT_DIR" ] && [ "$UE_OUTPUT_DIR" != "$INSTALLED_BUILD_PATH" ]; then
        echo "Copying installed build to output directory..."
        echo "Source: $INSTALLED_BUILD_PATH"
        echo "Destination: $UE_OUTPUT_DIR"
        echo ""
        
        rsync -a --progress "LocalBuilds/Engine/Linux/" "$UE_OUTPUT_DIR/" || {
            echo "WARNING: rsync failed, trying with basic copy..."
            cp -r "LocalBuilds/Engine/Linux/"* "$UE_OUTPUT_DIR/"
        }
        
        echo ""
        echo "You can use either location:"
        echo "  export UE_ROOT=$INSTALLED_BUILD_PATH"
        echo "  # OR"
        echo "  export UE_ROOT=$UE_OUTPUT_DIR"
    else
        echo "Using installed build directly from source location."
        echo ""
        echo "You can use this with:"
        echo "  export UE_ROOT=$INSTALLED_BUILD_PATH"
    fi
    
    echo ""
    echo "Next steps:"
    echo "  cd CloudMultiPlayerProject/docker/build"
    echo "  ./build_linux_in_docker.sh"
    echo ""
else
    echo "======================================================================="
    echo "WARNING: Build output not found at LocalBuilds/Engine/Linux"
    echo "======================================================================="
    echo "The build may have completed but the output is in a different location."
    echo "Please check the build logs and LocalBuilds directory."
    exit 1
fi

