#!/bin/bash

set -e
BUILD_TYPE=${1:-debug}

echo "Building project ($BUILD_TYPE)..."
if ! make BUILD=$BUILD_TYPE; then
	echo "Build failed with erro code: $?"
	exit 1
fi

echo "Done"
echo "All assemblies built successfully."
