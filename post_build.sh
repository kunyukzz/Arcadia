#!/bin/bash

echo "Compile Shaders..."

echo "Create Builtin.MaterialShader.vert.spv..."
$VULKAN_SDK/bin/glslc -fshader-stage=vert assets/shaders/Builtin.MaterialShader.vert.glsl -o assets/shaders/Builtin.MaterialShader.vert.spv
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
	echo "Error: "$ERRORLEVEL && exit
fi

echo "Create Builtin.MaterialShader.frag.spv..."
$VULKAN_SDK/bin/glslc -fshader-stage=frag assets/shaders/Builtin.MaterialShader.frag.glsl -o assets/shaders/Builtin.MaterialShader.frag.spv
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
	echo "Error: "$ERRORLEVEL && exit
fi

echo "Done"

