#include <engine/backend/vulkan_helpers.h>

uint32_t FindMemoryType(
    const VulkanDevice              device                  [static 1],
    const uint32_t                  type_filter, 
    const VkMemoryPropertyFlags     properties
) {
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(device->physical_device, &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    return UINT32_MAX;
}

bool CreateVulkanImageMemories(
    const VulkanDevice                  device                  [static 1],
    const uint32_t                      image_count,
    const VkImage                       images                  [static image_count],
    VkDeviceMemory                      image_memories          [static image_count]
) {
    for (uint32_t i = 0; i < image_count; i++) {
        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(device->handle, images[i], &memory_requirements);
        
        const VkMemoryAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memory_requirements.size,
            .memoryTypeIndex = FindMemoryType(device, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        };

        if (alloc_info.memoryTypeIndex == UINT32_MAX) {
            ROSINA_LOG_ERROR("Could not find memory type index");
            for (uint32_t j = 0; j < i; j++) {
                vkFreeMemory(device->handle, image_memories[j], NULL);
            }
            return true;
        }

        VK_ERROR_HANDLE(vkAllocateMemory(device->handle, &alloc_info, NULL, image_memories + i), {
            for (uint32_t j = 0; j < i; j++) {
                vkFreeMemory(device->handle, image_memories[j], NULL);
            }
            return true;
        });

        VK_ERROR_HANDLE(vkBindImageMemory(device->handle, images[i], image_memories[i], 0), {
            for (uint32_t j = 0; j < i; j++) {
                vkFreeMemory(device->handle, image_memories[j], NULL);
            }
            return true;
		});
    }

    return false;
}

bool CreateVulkanImageViews(
    const VulkanDevice                  device                  [static 1],
    const VulkanImageViewsCreateInfo    create_info             [static 1],
    VkImageView*                        image_views
) {
    for (uint32_t i = 0; i < create_info->image_count; i++) {
		create_info->vk_image_create_info->image = create_info->images[i];
		VK_ERROR_HANDLE(vkCreateImageView(device->handle, create_info->vk_image_create_info, NULL, image_views + i), {
			for (uint32_t j = 0; j < i; j++) {
                vkDestroyImageView(device->handle, image_views[i], NULL);
            }
			return true;
		});
	}
    return false;
}

void DestroyVulkanBuffer(
    const VulkanDevice              device                  [static 1], 
    VulkanBuffer                    buffer                  [static 1]
) {
    vkFreeMemory(device->handle, buffer->memory, NULL);
    vkDestroyBuffer(device->handle, buffer->handle, NULL);
}

bool CreateVulkanBuffer(
    const VulkanDevice              device                  [static 1],
    const VulkanBufferCreateInfo    create_info             [static 1],
    VulkanBuffer                    buffer                  [static 1]
) {
    {
        const VkBufferCreateInfo buffer_create_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .size = create_info->size,
            .usage = create_info->usage,
            .sharingMode = create_info->mode,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = NULL
        };

        VK_ERROR_RETURN(vkCreateBuffer(device->handle, &buffer_create_info, NULL, &buffer->handle), true);
    }

    {
        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(device->handle, buffer->handle, &mem_requirements);
        
        const VkMemoryAllocateInfo buffer_alloc_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = NULL,
            .allocationSize = create_info->size,
            .memoryTypeIndex = FindMemoryType(device, mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        };

        if (buffer_alloc_info.memoryTypeIndex == UINT32_MAX) {
            vkDestroyBuffer(device->handle, buffer->handle, NULL);
            return true;
        }

        VK_ERROR_HANDLE(vkAllocateMemory(device->handle, &buffer_alloc_info, NULL, &buffer->memory), {
            vkDestroyBuffer(device->handle, buffer->handle, NULL);
            return true;
        });

        VK_ERROR_HANDLE(vkBindBufferMemory(device->handle, buffer->handle, buffer->memory, 0), {
            vkFreeMemory(device->handle, buffer->memory, NULL);
            vkDestroyBuffer(device->handle, buffer->handle, NULL);
            return true;
        });
    }

    return false;
}

bool CreateVulkanStagingBuffer(
    const VulkanDevice                      device                  [static 1],
    const VkDeviceSize                      size,
    VulkanBuffer                            buffer                  [static 1]
) {
    const VulkanBufferCreateInfo create_info = {
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .mode = VK_SHARING_MODE_EXCLUSIVE,
        .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };
    return CreateVulkanBuffer(device, &create_info, buffer);
}

bool CreateVulkanVertexBuffer(
    const VulkanDevice                      device                  [static 1],
    const VkDeviceSize                      size,
    VulkanBuffer                            buffer                  [static 1]
) {
    const VulkanBufferCreateInfo create_info = {
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .mode = VK_SHARING_MODE_EXCLUSIVE,
        .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    };
    return CreateVulkanBuffer(device, &create_info, buffer);
}

bool CreateVulkanIndexBuffer(
    const VulkanDevice                      device                  [static 1],
    const VkDeviceSize                      size,
    VulkanBuffer                            buffer                  [static 1]
) {
    const VulkanBufferCreateInfo create_info = {
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .mode = VK_SHARING_MODE_EXCLUSIVE,
        .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    };
    return CreateVulkanBuffer(device, &create_info, buffer);
}

bool CreateVulkanUniformBuffer(
    const VulkanDevice                      device                  [static 1],
    const VkDeviceSize                      size,
    VulkanBuffer                            buffer                  [static 1]
) {
    const VulkanBufferCreateInfo create_info = {
        .size = size,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .mode = VK_SHARING_MODE_EXCLUSIVE,
        .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };
    return CreateVulkanBuffer(device, &create_info, buffer);
}