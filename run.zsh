#!/bin/zsh

EXECUTABLE=cratchit
ERROR_LIMIT=3
SEQUENTIAL_BUILD=true

# Read current preset from file, default to conan-debug if file doesn't exist
if [[ -f "cmake_preset.txt" ]]; then
    PRESET_NAME=$(cat cmake_preset.txt)
    echo "run.zsh: Using preset from cmake_preset.txt: $PRESET_NAME"
    
    # Extract build type from preset name for workspace directory
    case $PRESET_NAME in
        "conan-debug") BUILD_TYPE="Debug" ;;
        "conan-release") BUILD_TYPE="Release" ;;
        "conan-relwithdebinfo") BUILD_TYPE="RelWithDebInfo" ;;
        "conan-minsizerel") BUILD_TYPE="MinSizeRel" ;;
        *) 
            echo "run.zsh: Error: Unknown preset in cmake_preset.txt: $PRESET_NAME"
            echo "run.zsh: Run './init_tool_chain.zsh [Debug|Release|RelWithDebInfo|MinSizeRel]' to fix"
            exit 1
            ;;
    esac
else
    PRESET_NAME="conan-debug"
    BUILD_TYPE="Debug"
    echo "run.zsh: Warning: cmake_preset.txt not found, defaulting to conan-debug"
    echo "run.zsh: Run './init_tool_chain.zsh [Debug|Release|RelWithDebInfo|MinSizeRel]' first to set preset"
fi

# Define the workspace directory
WORKSPACE_DIR="workspace"

echo "run.zsh: Running $BUILD_TYPE build with preset $PRESET_NAME..."

# Create the workspace directory if it doesn't exist
echo "run.zsh: Creating workspace directory..."
mkdir -p "$WORKSPACE_DIR"

# Configure with error limit, then build
echo "run.zsh: Configuring with error limit..."
cmake --preset $PRESET_NAME -DMAX_COMPILE_ERRORS=$ERROR_LIMIT

echo "run.zsh: Building the project with cmake..."
if [[ "$SEQUENTIAL_BUILD" == "true" ]]; then
    echo "run.zsh: Using sequential build (SEQUENTIAL_BUILD=$SEQUENTIAL_BUILD)"
    cmake --build --preset $PRESET_NAME --parallel 1
else
    echo "run.zsh: Using parallel build (SEQUENTIAL_BUILD=$SEQUENTIAL_BUILD)"
    cmake --build --preset $PRESET_NAME
fi

# Check if the build was successful
if [[ $? -ne 0 ]]; then
    echo "run.zsh: Build failed. Exiting."
    exit 1
fi

# Copy the built binary to the workspace directory
echo "run.zsh: Copying the '$EXECUTABLE' binary to the workspace directory..."
cp "build/$BUILD_TYPE/$EXECUTABLE" "$WORKSPACE_DIR/"

# Change to the workspace directory
cd "$WORKSPACE_DIR"

./$EXECUTABLE "$@"
