#!/bin/zsh

# Exit the script on any command failure
set -e

conan install . --build=missing
cmake --preset conan-release
echo "Toolchain initialization complete."
echo "You can now build the project using:"
echo "  > 'cmake --build .' inside the 'build' directory."
echo "  > 'cmake --build build/Release' from here"
