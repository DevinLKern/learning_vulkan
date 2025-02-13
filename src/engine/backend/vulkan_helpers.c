#include <assert.h>
#include <engine/backend/vulkan_helpers.h>
#include <stdlib.h>

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

uint32_t FindQueueFamilyIndex(const VulkanDevice device[static 1], const QueueCapabilityFlags flags, const uint32_t count)
{
    uint32_t queue_family_property_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device->physical_device, &queue_family_property_count, NULL);

    VkQueueFamilyProperties* const queue_family_properties = calloc(queue_family_property_count, sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device->physical_device, &queue_family_property_count, queue_family_properties);

    for (uint32_t i = 0; i < queue_family_property_count; i++)
    {
        if ((flags & QUEUE_CAPABLITY_FLAG_GRAPHICS_BIT) && !queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            continue;
        }

        if ((flags & QUEUE_CAPABLITY_FLAG_TRANSFER_BIT) && !(queue_family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT))
        {
            continue;
        }

        if ((flags & QUEUE_CAPABLITY_FLAG_COMPUTE_BIT) && !(queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
        {
            continue;
        }

        VkBool32 surface_supported = VK_FALSE;
        VK_ERROR_HANDLE(vkGetPhysicalDeviceSurfaceSupportKHR(device->physical_device, i, device->surface, &surface_supported), { return UINT32_MAX; });
        if ((flags & QUEUE_CAPABLITY_FLAG_PRESENT_BIT) && surface_supported != VK_TRUE)
        {
            continue;
        }

        free(queue_family_properties);
        return i;
    }

    free(queue_family_properties);
    return UINT32_MAX;
}