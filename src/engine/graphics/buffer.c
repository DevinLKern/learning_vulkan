#include <engine/graphics/buffer.h>

#include <assert.h>
#include <string.h>

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
            .allocationSize  = (VkDeviceSize)size,
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
            case BUFFER_MEMORY_VERTEX_BUFFER_COMPONENT:
                vkDestroyBuffer(renderer->device.handle, memory->vertex_buffer, NULL);
                memory->vertex_buffer = VK_NULL_HANDLE;
                break;
            case BUFFER_MEMORY_INDEX_BUFFER_COMPONENT:
                vkDestroyBuffer(renderer->device.handle, memory->index_buffer, NULL);
                memory->index_buffer = VK_NULL_HANDLE;
                break;
            case BUFFER_MEMORY_UNIFORM_BUFFER_COMPONENT:
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

VkDeviceSize VkDeviceSize_RoundUpTo(const VkDeviceSize x, const VkDeviceSize y)
{
    VkDeviceSize z = x % y;
    if (z == 0) return x;
    z = y - z;
    return x + z;
}

BufferMemory BufferMemory_Create(const Renderer renderer[static 1], BufferMemoryCreateInfo buffer_memory_create_info[static 1])
{
    BufferMemory memory = {.component_count = 0,
                           .components      = {},
                           .handle          = VK_NULL_HANDLE,
                           .vertex_buffer   = VK_NULL_HANDLE,
                           .index_buffer    = VK_NULL_HANDLE,
                           .uniform_buffer  = VK_NULL_HANDLE};

    // create buffers
    {
        VkBufferCreateInfo buffer_create_info = {.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                                 .pNext                 = NULL,
                                                 .flags                 = 0,
                                                 .size                  = buffer_memory_create_info->vertex_buffer_capacity,
                                                 .usage                 = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                 .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
                                                 .queueFamilyIndexCount = 0,
                                                 .pQueueFamilyIndices   = NULL};
        if (buffer_memory_create_info->vertex_buffer_capacity > 0)
        {
            VK_ERROR_HANDLE(vkCreateBuffer(renderer->device.handle, &buffer_create_info, NULL, &memory.vertex_buffer), {
                BufferMemory_Cleanup(renderer, &memory);
                return memory;
            });
            memory.components[memory.component_count++] = BUFFER_MEMORY_VERTEX_BUFFER_COMPONENT;
        }

        if (buffer_memory_create_info->index_buffer_capacity > 0)
        {
            buffer_create_info.size  = buffer_memory_create_info->index_buffer_capacity;
            buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

            VK_ERROR_HANDLE(vkCreateBuffer(renderer->device.handle, &buffer_create_info, NULL, &memory.index_buffer), {
                BufferMemory_Cleanup(renderer, &memory);
                return memory;
            });
            memory.components[memory.component_count++] = BUFFER_MEMORY_INDEX_BUFFER_COMPONENT;
        }

        if (buffer_memory_create_info->uniform_buffer_capacity > 0)
        {
            buffer_create_info.size  = buffer_memory_create_info->uniform_buffer_capacity;
            buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

            VK_ERROR_HANDLE(vkCreateBuffer(renderer->device.handle, &buffer_create_info, NULL, &memory.uniform_buffer), {
                BufferMemory_Cleanup(renderer, &memory);
                return memory;
            });
            memory.components[memory.component_count++] = BUFFER_MEMORY_UNIFORM_BUFFER_COMPONENT;
        }
    }

    // allocate memory
    {
        uint32_t memory_type_bits = 0;
        VkMemoryRequirements reqs;
        VkDeviceSize offset = 0;
        VkDeviceSize size = 0;

        if (buffer_memory_create_info->vertex_buffer_capacity > 0)
        {
            vkGetBufferMemoryRequirements(renderer->device.handle, memory.vertex_buffer, &reqs);
            memory_type_bits |= reqs.memoryTypeBits;
            offset = VkDeviceSize_RoundUpTo(size, reqs.alignment);
            size = offset + reqs.size;
        }

        if (buffer_memory_create_info->index_buffer_capacity > 0)
        {
            vkGetBufferMemoryRequirements(renderer->device.handle, memory.index_buffer, &reqs);
            memory_type_bits |= reqs.memoryTypeBits;
            offset = VkDeviceSize_RoundUpTo(size, reqs.alignment);
            size = offset + reqs.size;
        }

        if (buffer_memory_create_info->uniform_buffer_capacity > 0)
        {
            vkGetBufferMemoryRequirements(renderer->device.handle, memory.uniform_buffer, &reqs);
            memory_type_bits |= reqs.memoryTypeBits;
            offset = VkDeviceSize_RoundUpTo(size, reqs.alignment);
            size = offset + reqs.size;
        }

        const VkMemoryAllocateInfo buffer_alloc_info = {
            .sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext          = NULL,
            .allocationSize = size,
            .memoryTypeIndex = FindMemoryType(renderer->device.physical_device, memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        };

        VK_ERROR_HANDLE(vkAllocateMemory(renderer->device.handle, &buffer_alloc_info, NULL, &memory.handle), {
            BufferMemory_Cleanup(renderer, &memory);
            return memory;
        });

        memory.components[memory.component_count++] = BUFFER_MEMORY_HANDLE_COMPONENT;
    }

    // bind offsets
    {
        VkMemoryRequirements reqs;
        VkDeviceSize offset = 0;
        VkDeviceSize size = 0;

        if (buffer_memory_create_info->vertex_buffer_capacity > 0)
        {
            vkGetBufferMemoryRequirements(renderer->device.handle, memory.vertex_buffer, &reqs);
            offset = VkDeviceSize_RoundUpTo(size, reqs.alignment);
            size = offset + reqs.size;
            VK_ERROR_HANDLE(vkBindBufferMemory(renderer->device.handle, memory.vertex_buffer, memory.handle, offset), {
                BufferMemory_Cleanup(renderer, &memory);
                return memory;
            });
        }

        if (buffer_memory_create_info->index_buffer_capacity > 0)
        {
            vkGetBufferMemoryRequirements(renderer->device.handle, memory.index_buffer, &reqs);
            offset = VkDeviceSize_RoundUpTo(size, reqs.alignment);
            size = offset + reqs.size;
            VK_ERROR_HANDLE(vkBindBufferMemory(renderer->device.handle, memory.index_buffer, memory.handle, offset), {
                BufferMemory_Cleanup(renderer, &memory);
                return memory;
            });
        }

        if (buffer_memory_create_info->uniform_buffer_capacity > 0)
        {
            vkGetBufferMemoryRequirements(renderer->device.handle, memory.uniform_buffer, &reqs);
            offset = VkDeviceSize_RoundUpTo(size, reqs.alignment);
            size = offset + reqs.size;
            VK_ERROR_HANDLE(vkBindBufferMemory(renderer->device.handle, memory.uniform_buffer, memory.handle, offset), {
                BufferMemory_Cleanup(renderer, &memory);
                return memory;
            });
        }
    }

    return memory;
}

