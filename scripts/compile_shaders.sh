
if [ ! -d "compiled_shaders" ]; then
    mkdir compiled_shaders
fi

glslc ./shaders/shader.vert -o ./compiled_shaders/vertex.spv;
glslc ./shaders/shader.frag -o ./compiled_shaders/fragment.spv;