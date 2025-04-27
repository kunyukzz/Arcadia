#!/bin/bash

set -e
BUILD_TYPE=${1:-debug}

echo "Clean previous build..."
make clean

echo "Building project ($BUILD_TYPE)..."
if ! make BUILD=$BUILD_TYPE; then
	echo "Build failed with erro code: $?"
	exit 1
fi

if [ -f ./post_build.sh ]; then
    echo "Running post build script..."
    ./post_build.sh
else
    echo "No post build script found, skipping."
fi

echo "Done"
echo "All assemblies built successfully."
