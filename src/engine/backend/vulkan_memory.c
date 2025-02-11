#include <assert.h>
#include <engine/backend/vulkan_helpers.h>

uint32_t FindMemoryType(const VkPhysicalDevice physical_device, const uint32_t type_filter, const VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
    {
        if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    return UINT32_MAX;
}

bool CreateVulkanImageMemories(const VulkanDevice device[static 1], const uint32_t image_count, const VkImage images[static image_count],
                               VkDeviceMemory image_memories[static image_count])
{
    for (uint32_t i = 0; i < image_count; i++)
    {
        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(device->handle, images[i], &memory_requirements);

        const VkMemoryAllocateInfo alloc_info = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize  = memory_requirements.size,
            .memoryTypeIndex = FindMemoryType(device->physical_device, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

        if (alloc_info.memoryTypeIndex == UINT32_MAX)
        {
            ROSINA_LOG_ERROR("Could not find memory type index");
            for (uint32_t j = 0; j < i; j++)
            {
                vkFreeMemory(device->handle, image_memories[j], NULL);
            }
            return true;
        }

        VK_ERROR_HANDLE(vkAllocateMemory(device->handle, &alloc_info, NULL, image_memories + i), {
            for (uint32_t j = 0; j < i; j++)
            {
                vkFreeMemory(device->handle, image_memories[j], NULL);
            }
            return true;
        });

        VK_ERROR_HANDLE(vkBindImageMemory(device->handle, images[i], image_memories[i], 0), {
            for (uint32_t j = 0; j < i; j++)
            {
                vkFreeMemory(device->handle, image_memories[j], NULL);
            }
            return true;
        });
    }

    return false;
}

bool CreateVulkanImageViews(const VulkanDevice device[static 1], const VulkanImageViewsCreateInfo create_info[static 1], VkImageView* image_views)
{
    for (uint32_t i = 0; i < create_info->image_count; i++)
    {
        create_info->vk_image_create_info->image = create_info->images[i];
        VK_ERROR_HANDLE(vkCreateImageView(device->handle, create_info->vk_image_create_info, NULL, image_views + i), {
            for (uint32_t j = 0; j < i; j++)
            {
                vkDestroyImageView(device->handle, image_views[i], NULL);
            }
            return true;
        });
    }
    return false;
}