#!/bin/bash
# Script to check the progress of a UE Linux build

UE_SOURCE="${UE_SOURCE:-$HOME/projects/DroneProject/UE}"

echo "======================================================================="
echo "Checking UE Linux Build Progress"
echo "======================================================================="
echo "Source: $UE_SOURCE"
echo ""

# Check if Setup.sh has run
if [ -d "$UE_SOURCE/Engine/Extras/ThirdPartyNotUE" ]; then
    echo "✓ Setup.sh has run (dependencies directory exists)"
else
    echo "✗ Setup.sh has not run yet (no dependencies directory)"
fi

# Check for build artifacts
if [ -d "$UE_SOURCE/LocalBuilds/Engine/Linux" ]; then
    echo "✓ Installed build directory exists"
    SIZE=$(du -sh "$UE_SOURCE/LocalBuilds/Engine/Linux" 2>/dev/null | cut -f1)
    echo "  Size: $SIZE"
    
    # Check for key binaries
    if [ -f "$UE_SOURCE/LocalBuilds/Engine/Linux/Engine/Build/BatchFiles/Linux/Build.sh" ]; then
        echo "✓ Build appears complete (Build.sh found)"
    else
        echo "~ Build in progress (Build.sh not found yet)"
    fi
else
    echo "✗ No installed build directory yet"
fi

# Check for intermediate build files
if [ -d "$UE_SOURCE/Engine/Binaries/Linux" ]; then
    echo "✓ Engine binaries directory exists (build in progress)"
    SIZE=$(du -sh "$UE_SOURCE/Engine/Binaries/Linux" 2>/dev/null | cut -f1)
    echo "  Size: $SIZE"
else
    echo "✗ No engine binaries yet"
fi

# Check for intermediate files
if [ -d "$UE_SOURCE/Engine/Intermediate" ]; then
    echo "✓ Intermediate build files exist"
    SIZE=$(du -sh "$UE_SOURCE/Engine/Intermediate" 2>/dev/null | cut -f1)
    echo "  Size: $SIZE"
fi

echo ""
echo "To resume build, just run:"
echo "  cd CloudMultiPlayerProject/docker/build_ue"
echo "  ./build_ue_linux_from_mac.sh"
echo ""


