#ifndef SANDBOX_APPLICATION_H
#define SANDBOX_APPLICATION_H

#include <engine/frontend/graphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <unistd.h>  // sleep()
#include <utility/math.h>

typedef enum ApplicationComponent
{
    APPLICATION_LIBRARIES_COMPONENT,
    APPLICATION_RENDERER_COMPONENT,
    APPLICATION_MEMORY_ARENA_COMPONENT,
    APPLICATION_SHADER_COMPONENT,
    APPLICATION_BUFFER_MEMORY_COMPONENT,
    APPLICATION_COMPONENT_COUNT
} ApplicationComponent;

typedef struct Application
{
    uint32_t component_count;
    ApplicationComponent components[APPLICATION_COMPONENT_COUNT];
    Renderer renderer;
    MemoryArena arena;
    VertexBufferObject vbo;
    IndexBufferObject ibo;
    Shader shader;
    BufferMemory buffer_memory;
} Application;

void Application_Cleanup(Application application[static 1]);

Application Application_Create();

void Application_Run(Application application[static 1]);

#endif