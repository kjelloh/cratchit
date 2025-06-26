#!/bin/zsh

# Define the build directory and workspace directory
BUILD_DIR="build/Release"
WORKSPACE_DIR="workspace"

# Create the workspace directory if it doesn't exist
echo "Creating workspace directory..."
mkdir -p "$WORKSPACE_DIR"

# Build the project using cmake
echo "Building the project with cmake..."
cmake --build "$BUILD_DIR"

# Check if the build was successful
if [[ $? -ne 0 ]]; then
    echo "Build failed. Exiting."
    exit 1
fi

# Copy the built binary (cratchit) to the workspace directory
echo "Copying the 'cratchit' binary to the workspace directory..."
cp "$BUILD_DIR/cratchit" "$WORKSPACE_DIR/"

# Change to the workspace directory
cd "$WORKSPACE_DIR"

./cratchit "$@"
#ASAN_OPTIONS=log_path=asan_log ./cratchit "$@" 2>stderr.log

