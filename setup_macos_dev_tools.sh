#!/bin/bash
# Copyright (C) Microsoft Corporation. 
# Copyright (C) 2025 IAMAI CONSULTING CORP
# MIT License.

set -e

echo "======================================================================="
echo "Setting up development tools for ProjectAirSim on macOS..."
echo "======================================================================="

# Check if Homebrew is installed
if ! command -v brew &> /dev/null; then
    echo "Homebrew not found. Installing Homebrew..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    
    # Add Homebrew to PATH for this session
    if [[ $(uname -m) == "arm64" ]]; then
        echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zprofile
        eval "$(/opt/homebrew/bin/brew shellenv)"
    else
        echo 'eval "$(/usr/local/bin/brew shellenv)"' >> ~/.zprofile
        eval "$(/usr/local/bin/brew shellenv)"
    fi
else
    echo "Homebrew found. Updating..."
    brew update
fi

# Install prerequisites
echo "Installing build prerequisites..."
brew install \
    cmake \
    ninja \
    llvm \
    rsync

# Check if Xcode command line tools are installed
if ! xcode-select -p &> /dev/null; then
    echo "Xcode command line tools not found. Installing..."
    xcode-select --install
    echo "Please complete the Xcode command line tools installation in the popup window."
    echo "After installation completes, run this script again."
    exit 1
else
    echo "Xcode command line tools found."
fi

# Set up environment variables for clang
echo "Setting up environment variables..."
if [[ $(uname -m) == "arm64" ]]; then
    # Apple Silicon
    export CC=/opt/homebrew/opt/llvm/bin/clang
    export CXX=/opt/homebrew/opt/llvm/bin/clang++
    echo 'export CC=/opt/homebrew/opt/llvm/bin/clang' >> ~/.zshrc
    echo 'export CXX=/opt/homebrew/opt/llvm/bin/clang++' >> ~/.zshrc
else
    # Intel Mac
    export CC=/usr/local/opt/llvm/bin/clang
    export CXX=/usr/local/opt/llvm/bin/clang++
    echo 'export CC=/usr/local/opt/llvm/bin/clang' >> ~/.zshrc
    echo 'export CXX=/usr/local/opt/llvm/bin/clang++' >> ~/.zshrc
fi

echo "======================================================================="
echo "Development tools setup completed!"
echo "======================================================================="
echo ""
echo "Next steps:"
echo "1. Restart your terminal or run: source ~/.zshrc"
echo "2. Run: ./build_macos.sh all_no_test"
echo ""
echo "Optional: If you want to build Unreal Engine components, set UE_ROOT:"
echo "export UE_ROOT=/path/to/your/UnrealEngine"
echo ""
echo "For Python development, you may also want to install Python dependencies:"
echo "cd client/python/projectairsim && pip install -r requirements.txt"
