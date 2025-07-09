#!/bin/zsh

# Exit the script on any command failure
set -e


# Parse arguments: use Debug as default, accept Debug, Release, RelWithDebInfo, MinSizeRel
BUILD_TYPE=${1:-Debug}

# Validate build type and map to preset name
case $BUILD_TYPE in
    "Debug") PRESET_NAME="conan-debug" ;;
    "Release") PRESET_NAME="conan-release" ;;
    "RelWithDebInfo") PRESET_NAME="conan-relwithdebinfo" ;;
    "MinSizeRel") PRESET_NAME="conan-minsizerel" ;;
    *) 
        echo "Error: BUILD_TYPE must be 'Debug', 'Release', 'RelWithDebInfo', or 'MinSizeRel'"
        echo "Usage: $0 [Debug|Release|RelWithDebInfo|MinSizeRel]"
        exit 1
        ;;
esac

echo "🔧 Initializing toolchain for $BUILD_TYPE build..."

# Install dependencies for the specified build type (also updates cmake presets)
echo "📦 Installing dependencies with Conan..."
conan install . --settings=compiler.cppstd=23 --settings=build_type=$BUILD_TYPE --build=missing

# Configure CMake with the generated preset
echo "⚙️ Configuring CMake with preset: $PRESET_NAME"
cmake --preset $PRESET_NAME

# Save the current preset for run.zsh script
echo "$PRESET_NAME" > cmake_preset.txt

echo "✅ Toolchain initialization complete for $BUILD_TYPE build."
echo "Current preset saved to cmake_preset.txt: $PRESET_NAME"
echo ""
echo "Build options:"
echo "  1. 'cmake --build --preset $PRESET_NAME'"
echo "  2. 'cmake --build --preset \$(cat cmake_preset.txt)'"
echo "  3. './run.zsh' (builds and runs the project)"
