#include <assert.h>
#include <engine/frontend/graphics.h>

VertexBuffer VertexBuffer_Create(Renderer renderer[static 1], uint64_t size)
{
    VertexBuffer buffer = {.handle = VK_NULL_HANDLE, .memory = VK_NULL_HANDLE};

    assert(renderer != NULL);
    assert(size > 0);
    {
        const VkBufferCreateInfo buffer_create_info = {.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                                       .pNext                 = NULL,
                                                       .flags                 = 0,
                                                       .size                  = size,
                                                       .usage                 = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                       .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
                                                       .queueFamilyIndexCount = 0,
                                                       .pQueueFamilyIndices   = NULL};

        VK_ERROR_RETURN(vkCreateBuffer(renderer->device.handle, &buffer_create_info, NULL, &buffer.handle), buffer);
    }

    {
        VkMemoryRequirements reqs;
        vkGetBufferMemoryRequirements(renderer->device.handle, buffer.handle, &reqs);

        const VkMemoryAllocateInfo buffer_alloc_info = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext           = NULL,
            .allocationSize  = size,
            .memoryTypeIndex = FindMemoryType(renderer->device.physical_device, reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

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
            vkFreeMemory(renderer->device.handle, buffer.memory, NULL);
            vkDestroyBuffer(renderer->device.handle, buffer.handle, NULL);
            buffer.handle = VK_NULL_HANDLE;
            buffer.memory = VK_NULL_HANDLE;
            return buffer;
        });
    }

    return buffer;
}

IndexBuffer IndexBuffer_Create(Renderer renderer[static 1], uint64_t size)
{
    IndexBuffer buffer = {.handle = VK_NULL_HANDLE, .memory = VK_NULL_HANDLE};

    assert(renderer != NULL);
    assert(size > 0);
    {
        const VkBufferCreateInfo buffer_create_info = {.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                                       .pNext                 = NULL,
                                                       .flags                 = 0,
                                                       .size                  = size,
                                                       .usage                 = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                       .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
                                                       .queueFamilyIndexCount = 0,
                                                       .pQueueFamilyIndices   = NULL};

        VK_ERROR_RETURN(vkCreateBuffer(renderer->device.handle, &buffer_create_info, NULL, &buffer.handle), buffer);
    }

    {
        VkMemoryRequirements reqs;
        vkGetBufferMemoryRequirements(renderer->device.handle, buffer.handle, &reqs);

        const VkMemoryAllocateInfo buffer_alloc_info = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext           = NULL,
            .allocationSize  = size,
            .memoryTypeIndex = FindMemoryType(renderer->device.physical_device, reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

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
            vkFreeMemory(renderer->device.handle, buffer.memory, NULL);
            vkDestroyBuffer(renderer->device.handle, buffer.handle, NULL);
            buffer.handle = VK_NULL_HANDLE;
            buffer.memory = VK_NULL_HANDLE;
            return buffer;
        });
    }

    return buffer;
}

UniformBuffer UniformBuffer_Create(Renderer renderer[static 1], uint64_t size)
{
    assert(renderer != NULL);
    assert(size > 0);

    UniformBuffer buffer = {.handle = VK_NULL_HANDLE, .memory = VK_NULL_HANDLE};

    {
        const VkBufferCreateInfo buffer_create_info = {.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                                       .pNext                 = NULL,
                                                       .flags                 = 0,
                                                       .size                  = size,
                                                       .usage                 = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                       .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
                                                       .queueFamilyIndexCount = 0,
                                                       .pQueueFamilyIndices   = NULL};

        VK_ERROR_RETURN(vkCreateBuffer(renderer->device.handle, &buffer_create_info, NULL, &buffer.handle), buffer);
    }

    {
        VkMemoryRequirements reqs;
        vkGetBufferMemoryRequirements(renderer->device.handle, buffer.handle, &reqs);

        const VkMemoryAllocateInfo buffer_alloc_info = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext           = NULL,
            .allocationSize  = size,
            .memoryTypeIndex = FindMemoryType(renderer->device.physical_device, reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

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
            vkFreeMemory(renderer->device.handle, buffer.memory, NULL);
            vkDestroyBuffer(renderer->device.handle, buffer.handle, NULL);
            buffer.handle = VK_NULL_HANDLE;
            buffer.memory = VK_NULL_HANDLE;
            return buffer;
        });
    }

    return buffer;
}