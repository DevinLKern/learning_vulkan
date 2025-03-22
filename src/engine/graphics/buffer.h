#ifndef BUFFER_MEMORY_H
#define BUFFER_MEMORY_H

#include <engine/graphics/renderer.h>

typedef struct StagingBuffer
{
    VkDeviceMemory memory;
    VkBuffer handle;
} StagingBuffer;

void StagingBuffer_Cleanup(const Renderer renderer[static 1], StagingBuffer buffer[static 1]);

StagingBuffer StagingBuffer_Create(const Renderer renderer[static 1], const uint64_t size);

typedef struct BufferMemoryCreateInfo
{
    VkDeviceSize vertex_buffer_capacity;
    VkDeviceSize index_buffer_capacity;
    VkDeviceSize uniform_buffer_capacity;
} BufferMemoryCreateInfo;

typedef enum BufferMemoryComponent
{
    BUFFER_MEMORY_VERTEX_BUFFER_COMPONENT,
    BUFFER_MEMORY_INDEX_BUFFER_COMPONENT,
    BUFFER_MEMORY_UNIFORM_BUFFER_COMPONENT,
    BUFFER_MEMORY_HANDLE_COMPONENT,
    BUFFER_MEMORY_COMPONENT_COUNT
} BufferMemoryComponent;

typedef struct BufferMemory
{
    uint32_t component_count;
    BufferMemoryComponent components[BUFFER_MEMORY_COMPONENT_COUNT];
    VkDeviceSize vertex_buffer_size;
    VkBuffer vertex_buffer;
    VkDeviceSize index_buffer_size;
    VkBuffer index_buffer;
    VkDeviceSize uniform_buffer_size;
    VkBuffer uniform_buffer;
    VkDeviceMemory handle;
} BufferMemory;

void BufferMemory_Cleanup(const Renderer renderer[static 1], BufferMemory memory[static 1]);

/**
 * Each buffer referenced in the BufferInfo struct must have the size field be > 0 and the offset field be UINT64_MAX.
 */
BufferMemory BufferMemory_Create(const Renderer renderer[static 1], BufferMemoryCreateInfo create_info[static 1]);

typedef struct BufferObject
{
    VkDeviceSize size;
    VkDeviceSize offset;
} BufferObject;

typedef BufferObject VertexBufferObject;
typedef BufferObject IndexBufferObject;
typedef BufferObject UniformBufferObject;

static inline VertexBufferObject VertexBufferObject_Create(const VkDeviceSize size, BufferMemory memory[static 1])
{
    const VertexBufferObject vbo = {.size = size, .offset = memory->vertex_buffer_size};
    memory->vertex_buffer_size += size;
    return vbo;
}

static inline IndexBufferObject IndexBufferObject_Create(const VkDeviceSize size, BufferMemory memory[static 1])
{
    const IndexBufferObject ibo = {.size = size, .offset = memory->index_buffer_size};
    memory->index_buffer_size += size;
    return ibo;
}

static inline UniformBufferObject UniformBufferObject_Create(const VkDeviceSize size, BufferMemory memory[static 1])
{
    const UniformBufferObject ubo = {.size = size, .offset = memory->uniform_buffer_size};
    memory->uniform_buffer_size += size;
    return ubo;
}

static inline void VertexBufferObject_Bind(const Renderer renderer[static 1], const BufferMemory memory[static 1], const VertexBufferObject vbo[static 1])
{
    vkCmdBindVertexBuffers(renderer->primary_command_buffers[renderer->frame_index], 0, 1, &memory->vertex_buffer, &vbo->offset);
}

static inline void IndexBufferObject_Bind(const Renderer renderer[static 1], const BufferMemory memory[static 1], const IndexBufferObject ibo[static 1])
{
    vkCmdBindIndexBuffer(renderer->primary_command_buffers[renderer->frame_index], memory->index_buffer, ibo->offset, VK_INDEX_TYPE_UINT32);
}


#endif
