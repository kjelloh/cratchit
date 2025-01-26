#!/bin/zsh

# Get the generator name from the parent folder's tool_chain.txt
GENERATOR_NAME=$(cat tool_chain.txt)

# Check if a generator name was read from the tool_chain.txt
if [ -z "$GENERATOR_NAME" ]; then
    echo "Error: No generator name found in 'tool_chain.txt'."
    exit 1
fi

# Define the sub-folder name based on the generator name
BUILD_FOLDER="$GENERATOR_NAME"


mkdir "$BUILD_FOLDER"
cd "$BUILD_FOLDER"

# Run CMake with the generator name and toolchain file
echo "Running CMake configuration..."
cmake -G "$GENERATOR_NAME" ..

echo "Build folder '$BUILD_FOLDER' is ready and dependencies have been installed."
