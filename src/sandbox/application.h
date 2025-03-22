#ifndef SANDBOX_APPLICATION_H
#define SANDBOX_APPLICATION_H

#include <engine/graphics/renderer.h>
#include <engine/graphics/shader.h>
#include <engine/graphics/image.h>

typedef enum ApplicationComponent
{
    APPLICATION_RENDERER_COMPONENT,
    APPLICATION_MEMORY_ARENA_COMPONENT,
    APPLICATION_SHADER_COMPONENT,
    APPLICATION_BUFFER_MEMORY_COMPONENT,
    APPLICATION_IMAGE_COMPONENT,
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
    Image image;
} Application;

void Application_Cleanup(Application application[static 1]);

Application Application_Create();

void Application_Run(Application application[static 1]);

#endif