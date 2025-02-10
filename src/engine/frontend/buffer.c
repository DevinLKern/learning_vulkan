#include <assert.h>
#include <engine/frontend/graphics.h>

StagingBuffer StagingBuffer_Create(const Renderer renderer[static 1], const uint64_t size)
{
    StagingBuffer buffer = {.handle = VK_NULL_HANDLE, .memory = VK_NULL_HANDLE};

    {
        const VkBufferCreateInfo buffer_create_info = {.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                                       .pNext                 = NULL,
                                                       .flags                 = 0,
                                                       .size                  = size,
                                                       .usage                 = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                       .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
                                                       .queueFamilyIndexCount = 0,
                                                       .pQueueFamilyIndices   = NULL};

        VK_ERROR_RETURN(vkCreateBuffer(renderer->device.handle, &buffer_create_info, NULL, &buffer.handle), buffer);
    }

    {
        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(renderer->device.handle, buffer.handle, &mem_requirements);

        const VkMemoryAllocateInfo buffer_alloc_info = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext           = NULL,
            .allocationSize  = size,
            .memoryTypeIndex = FindMemoryType(renderer->device.physical_device, mem_requirements.memoryTypeBits,
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)};

        if (buffer_alloc_info.memoryTypeIndex == UINT32_MAX)
        {
            vkDestroyBuffer(renderer->device.handle, buffer.handle, NULL);
            buffer.handle = VK_NULL_HANDLE;
            return buffer;
        }

        VK_ERROR_HANDLE(vkAllocateMemory(renderer->device.handle, &buffer_alloc_info, NULL, &buffer.memory), {
            vkDestroyBuffer(renderer->device.handle, buffer.handle, NULL);
            buffer.handle = VK_NULL_HANDLE;
            return buffer;
        });

        VK_ERROR_HANDLE(vkBindBufferMemory(renderer->device.handle, buffer.handle, buffer.memory, 0), {
            vkDestroyBuffer(renderer->device.handle, buffer.handle, NULL);
            buffer.handle = VK_NULL_HANDLE;
            vkFreeMemory(renderer->device.handle, buffer.memory, NULL);
            buffer.memory = VK_NULL_HANDLE;
            return buffer;
        });
    }

    return buffer;
}

void StagingBuffer_Cleanup(const Renderer renderer[static 1], StagingBuffer buffer[static 1])
{
    vkDestroyBuffer(renderer->device.handle, buffer->handle, NULL);
    buffer->handle = VK_NULL_HANDLE;
    vkFreeMemory(renderer->device.handle, buffer->memory, NULL);
    buffer->memory = VK_NULL_HANDLE;
}

void BufferMemory_Cleanup(const Renderer renderer[static 1], BufferMemory memory[static 1])
{
    while (memory->component_count > 0)
    {
        switch (memory->components[--memory->component_count])
        {
            case BUFFER_MEMORY_VERTEX_BUFFERS_COMPONENT:
                vkDestroyBuffer(renderer->device.handle, memory->vertex_buffer, NULL);
                memory->vertex_buffer = VK_NULL_HANDLE;
                break;
            case BUFFER_MEMORY_ibos_COMPONENT:
                vkDestroyBuffer(renderer->device.handle, memory->index_buffer, NULL);
                memory->index_buffer = VK_NULL_HANDLE;
                break;
            case BUFFER_MEMORY_UNIFORM_BUFFERS_COMPONENT:
                vkDestroyBuffer(renderer->device.handle, memory->uniform_buffer, NULL);
                memory->uniform_buffer = VK_NULL_HANDLE;
                break;
            case BUFFER_MEMORY_HANDLE_COMPONENT:
                vkFreeMemory(renderer->device.handle, memory->handle, NULL);
                memory->handle = VK_NULL_HANDLE;
                break;
            default:
                assert(false);
        }
    }
}

BufferMemory BufferMemory_Create(const Renderer renderer[static 1], const BufferInfo buffers[static 1])
{
    BufferMemory memory = {.component_count = 0,
                           .components      = {},
                           .handle          = VK_NULL_HANDLE,
                           .vertex_buffer   = VK_NULL_HANDLE,
                           .index_buffer    = VK_NULL_HANDLE,
                           .uniform_buffer  = VK_NULL_HANDLE};

    VkDeviceSize vertex_buffer_size  = 0;
    VkDeviceSize index_buffer_size   = 0;
    VkDeviceSize uniform_buffer_size = 0;
    {
        for (uint32_t i = 0; i < buffers->vbo_count; i++)
        {
            buffers->vbos[i].offset = vertex_buffer_size;
            vertex_buffer_size += buffers->vbos[i].size;
        }
        for (uint32_t i = 0; i < buffers->ibo_count; i++)
        {
            buffers->ibos[i].offset = index_buffer_size;
            index_buffer_size += buffers->ibos[i].size;
        }
        for (uint32_t i = 0; i < buffers->uniform_buffer_count; i++)
        {
            buffers->uniform_buffers[i].offset = uniform_buffer_size;
            uniform_buffer_size += buffers->uniform_buffers[i].size;
        }
    }

    // create buffers
    {
        VkBufferCreateInfo buffer_create_info = {.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                                 .pNext                 = NULL,
                                                 .flags                 = 0,
                                                 .size                  = vertex_buffer_size,
                                                 .usage                 = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                 .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
                                                 .queueFamilyIndexCount = 0,
                                                 .pQueueFamilyIndices   = NULL};
        if (buffers->vbo_count > 0)
        {
            VK_ERROR_HANDLE(vkCreateBuffer(renderer->device.handle, &buffer_create_info, NULL, &memory.vertex_buffer), {
                BufferMemory_Cleanup(renderer, &memory);
                return memory;
            });
            memory.components[memory.component_count++] = BUFFER_MEMORY_VERTEX_BUFFERS_COMPONENT;
        }

        if (buffers->ibo_count > 0)
        {
            buffer_create_info.size  = index_buffer_size;
            buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

            VK_ERROR_HANDLE(vkCreateBuffer(renderer->device.handle, &buffer_create_info, NULL, &memory.index_buffer), {
                BufferMemory_Cleanup(renderer, &memory);
                return memory;
            });
            memory.components[memory.component_count++] = BUFFER_MEMORY_ibos_COMPONENT;
        }

        if (buffers->uniform_buffer_count > 0)
        {
            buffer_create_info.size  = uniform_buffer_size;
            buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

            VK_ERROR_HANDLE(vkCreateBuffer(renderer->device.handle, &buffer_create_info, NULL, &memory.uniform_buffer), {
                BufferMemory_Cleanup(renderer, &memory);
                return memory;
            });
            memory.components[memory.component_count++] = BUFFER_MEMORY_UNIFORM_BUFFERS_COMPONENT;
        }
    }

    // allocate memory
    {
        uint32_t memory_type_bits = 0;
        VkMemoryRequirements reqs;

        if (buffers->vbo_count > 0)
        {
            vkGetBufferMemoryRequirements(renderer->device.handle, memory.vertex_buffer, &reqs);
            vertex_buffer_size += (reqs.alignment - (vertex_buffer_size % reqs.alignment)) % reqs.alignment;
            memory_type_bits |= reqs.memoryTypeBits;
        }

        if (buffers->ibo_count > 0)
        {
            vkGetBufferMemoryRequirements(renderer->device.handle, memory.index_buffer, &reqs);
            index_buffer_size += (reqs.alignment - (index_buffer_size % reqs.alignment)) % reqs.alignment;
            memory_type_bits |= reqs.memoryTypeBits;
        }

        if (buffers->uniform_buffer_count > 0)
        {
            vkGetBufferMemoryRequirements(renderer->device.handle, memory.uniform_buffer, &reqs);
            uniform_buffer_size += (reqs.alignment - (uniform_buffer_size % reqs.alignment)) % reqs.alignment;
            memory_type_bits |= reqs.memoryTypeBits;
        }

        const VkMemoryAllocateInfo buffer_alloc_info = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext           = NULL,
            .allocationSize  = vertex_buffer_size + index_buffer_size + uniform_buffer_size,
            .memoryTypeIndex = FindMemoryType(renderer->device.physical_device, memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

        VK_ERROR_HANDLE(vkAllocateMemory(renderer->device.handle, &buffer_alloc_info, NULL, &memory.handle), {
            BufferMemory_Cleanup(renderer, &memory);
            return memory;
        });

        memory.components[memory.component_count++] = BUFFER_MEMORY_HANDLE_COMPONENT;
    }

    // bind offsets
    {
        VkDeviceSize current_offset = 0;

        if (buffers->vbo_count > 0)
        {
            VK_ERROR_HANDLE(vkBindBufferMemory(renderer->device.handle, memory.vertex_buffer, memory.handle, current_offset), {
                BufferMemory_Cleanup(renderer, &memory);
                return memory;
            });
            current_offset += vertex_buffer_size;
        }

        if (buffers->ibo_count > 0)
        {
            VK_ERROR_HANDLE(vkBindBufferMemory(renderer->device.handle, memory.index_buffer, memory.handle, current_offset), {
                BufferMemory_Cleanup(renderer, &memory);
                return memory;
            });
            current_offset += index_buffer_size;
        }

        if (buffers->uniform_buffer_count > 0)
        {
            VK_ERROR_HANDLE(vkBindBufferMemory(renderer->device.handle, memory.uniform_buffer, memory.handle, current_offset), {
                BufferMemory_Cleanup(renderer, &memory);
                return memory;
            });
            current_offset += uniform_buffer_size;
        }
    }

    return memory;
}