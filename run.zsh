#!/bin/zsh

EXECUTABLE=cratchit

# Read current preset from file, default to conan-debug if file doesn't exist
if [[ -f "cmake_preset.txt" ]]; then
    PRESET_NAME=$(cat cmake_preset.txt)
    echo "Using preset from cmake_preset.txt: $PRESET_NAME"
    
    # Extract build type from preset name for workspace directory
    case $PRESET_NAME in
        "conan-debug") BUILD_TYPE="Debug" ;;
        "conan-release") BUILD_TYPE="Release" ;;
        "conan-relwithdebinfo") BUILD_TYPE="RelWithDebInfo" ;;
        "conan-minsizerel") BUILD_TYPE="MinSizeRel" ;;
        *) 
            echo "Error: Unknown preset in cmake_preset.txt: $PRESET_NAME"
            echo "Run './init_tool_chain.zsh [Debug|Release|RelWithDebInfo|MinSizeRel]' to fix"
            exit 1
            ;;
    esac
else
    PRESET_NAME="conan-debug"
    BUILD_TYPE="Debug"
    echo "Warning: cmake_preset.txt not found, defaulting to conan-debug"
    echo "Run './init_tool_chain.zsh [Debug|Release|RelWithDebInfo|MinSizeRel]' first to set preset"
fi

# Define the workspace directory
WORKSPACE_DIR="workspace"

echo "Running $BUILD_TYPE build with preset $PRESET_NAME..."

# Create the workspace directory if it doesn't exist
echo "Creating workspace directory..."
mkdir -p "$WORKSPACE_DIR"

# Build the project using cmake preset
echo "Building the project with cmake..."
cmake --build --preset $PRESET_NAME

# Check if the build was successful
if [[ $? -ne 0 ]]; then
    echo "Build failed. Exiting."
    exit 1
fi

# Copy the built binary to the workspace directory
echo "Copying the '$EXECUTABLE' binary to the workspace directory..."
cp "build/$BUILD_TYPE/$EXECUTABLE" "$WORKSPACE_DIR/"

# Change to the workspace directory
cd "$WORKSPACE_DIR"

./$EXECUTABLE "$@"
