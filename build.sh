#!/bin/bash

set echo on

echo "Clean project...."
make clean

echo "Building project..."
make

if ! make; then
	echo "Build failed with erro code: $?"
	exit 1
fi

echo "Done"
echo "All assemblies built successfully."
