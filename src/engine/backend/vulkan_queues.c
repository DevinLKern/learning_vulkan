#include <engine/backend/vulkan_helpers.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

static inline bool GetQueueFamilyIndex(
    const VkPhysicalDevice          physical_device,
    const VkSurfaceKHR              surface,
    const QueueCapabilityFlags      desired_capabilities,
    const uint32_t                  queue_family_property_count,
    VkQueueFamilyProperties*        queue_family_properties,
    uint32_t                        queue_index                     [static 1]
) {
    assert(*queue_index == UINT32_MAX);

    for (uint32_t i = 0; i < queue_family_property_count; i++) {
        if (queue_family_properties[i].queueCount == 0) {
            continue;
        }
        
        bool desired = (desired_capabilities & QUEUE_CAPABLITY_FLAG_GRAPHICS_BIT);
        bool supported = (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT);
        if (desired && !supported) {
            continue;
        }
        desired = (desired_capabilities & QUEUE_CAPABLITY_FLAG_TRANSFER_BIT);
        supported = (queue_family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT);
        if (desired && !supported) {
            continue;
        }
        desired = (desired_capabilities & QUEUE_CAPABLITY_FLAG_COMPUTE_BIT);
        supported = (queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT);
        if (desired && !supported) {
            continue;
        }

        desired = (desired_capabilities & QUEUE_CAPABLITY_FLAG_PRESENT_BIT);
        VkBool32 surface_supported = VK_FALSE;
        VK_ERROR_HANDLE(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &surface_supported), {
            return true;
        });
        if (desired && surface_supported != VK_TRUE) {
            continue;
        }

        queue_family_properties[i].queueCount--;
        *queue_index = i;

    }
    
    return *queue_index == UINT32_MAX;
}

bool FindQueueFamilyIndices(
    const VulkanDevice                      device                  [static 1],
    const FindQueueFamilyIndexInfo          select_info             [static 1],
    uint32_t* const                         queue_family_indices
) {
    assert(queue_family_indices != NULL);

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device->physical_device, &queue_family_count, NULL);

    VkQueueFamilyProperties* const queue_family_properties = calloc(queue_family_count, sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device->physical_device, &queue_family_count, queue_family_properties);

    for (uint32_t i = 0; i < select_info->queue_capability_count; i++) {
        if (GetQueueFamilyIndex(
            device->physical_device,
            device->surface,
            select_info->queue_capabilities[i], 
            queue_family_count,
            queue_family_properties,
            queue_family_indices + i
        )) {
            free(queue_family_properties);
            return true;
        }
    }
    
    free(queue_family_properties);
    return false;
}