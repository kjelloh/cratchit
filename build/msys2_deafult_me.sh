#!/bin/bash -v
BUILD_DIR=./build_msys2
if [ -d "$BUILD_DIR" ]
then 
	rm -rf "$BUILD_DIR"
fi
mkdir "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. -G"MSYS Makefiles"
