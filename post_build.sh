#!/bin/bash

mkdir -p bin/assets
mkdir -p bin/assets/shaders

echo "Compile Shaders..."

echo "Create Builtin.ObjShader.vert.spv"
$VULKAN_SDK/bin/glslc -fshader-stage=vert assets/shaders/Builtin.ObjShader.vert.glsl -o bin/assets/shaders/Builtin.ObjShader.vert.spv
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
	echo "Error: "$ERRORLEVEL && exit
fi

echo "Create Builtin.ObjShader.frag.spv"
$VULKAN_SDK/bin/glslc -fshader-stage=frag assets/shaders/Builtin.ObjShader.frag.glsl -o bin/assets/shaders/Builtin.ObjShader.frag.spv
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
	echo "Error: "$ERRORLEVEL && exit
fi

echo "Copy Assets..."
cp -R "assets" "bin"

echo "Done"
