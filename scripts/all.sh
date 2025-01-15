#! /bin/bash

COMPILED_SHADER_DIR=compiled_shaders
if [ -d "$COMPILED_SHADER_DIR" ]; then
    sudo rm -r $COMPILED_SHADER_DIR
fi

BUILD_DIR=build

if [ -d "$BUILD_DIR" ]; then
    sudo rm -r $BUILD_DIR
fi

echo "cleaned project!"

COMPILED_SHADER_DIR=compiled_shaders
if [ ! -d "$COMPILED_SHADER_DIR" ]; then
    mkdir $COMPILED_SHADER_DIR
fi

glslc ./shaders/shader.vert -o ./compiled_shaders/vertex.spv;
glslc ./shaders/shader.frag -o ./compiled_shaders/fragment.spv;

echo "compiled shaders!"

cmake -S . -B build

echo "configured prject!"

cd build
scan-build -V make

echo "project built!"

./sandbox_game