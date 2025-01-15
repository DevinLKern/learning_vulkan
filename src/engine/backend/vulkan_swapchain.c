#include <engine/backend/vulkan_helpers.h>


#include <assert.h>
#include <stdlib.h>


void DestroyVulkanSwapchain(
    const VulkanDevice              device                  [static 1],
    VulkanSwapchain                 swapchain               [static 1]
) {
    vkDestroySwapchainKHR(device->handle, swapchain->handle, NULL);
}

static inline bool SelectVkPresentModeKHR(
    const VulkanDevice              device                  [static 1], 
    VkPresentModeKHR                present_mode            [static 1]
) {
    assert(*present_mode == VK_PRESENT_MODE_FIFO_KHR);

    uint32_t supported_mode_count = 0;
    VK_ERROR_RETURN(vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical_device, device->surface, &supported_mode_count, NULL), true);
    VkPresentModeKHR* const supported_modes = calloc(supported_mode_count, sizeof(VkPresentModeKHR));
    VK_ERROR_HANDLE(vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical_device, device->surface, &supported_mode_count, supported_modes), {
        free(supported_modes);
        return true;
    });

    for (uint32_t i = 0; i < supported_mode_count; i++) {
        if (supported_modes[i] != VK_PRESENT_MODE_MAILBOX_KHR) {
            continue;
        }
        *present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
        break;
    }

    free(supported_modes);
    return false;
}

static inline bool SelectSwapchainExtent(
    const VulkanDevice              device                  [static 1],
    const uint32_t                  width,
    const uint32_t                  height,
    VkExtent2D                      extent                  [static 1]
) {
    VkSurfaceCapabilitiesKHR capabilities = {};
    VK_ERROR_RETURN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical_device, device->surface, &capabilities), true);

    // Here is an example of how to do this differently?
    // https://github.com/LunarG/VulkanSamples/blob/master/API-Samples/05-init_swapchain/05-init_swapchain.cpp#L170
    if (capabilities.currentExtent.width == UINT32_MAX) {
        const uint32_t min_width = capabilities.minImageExtent.width > width 
            ? capabilities.minImageExtent.width 
            : width;
        extent->width = capabilities.maxImageExtent.width < min_width 
            ? capabilities.maxImageExtent.width 
            : min_width;
            
        const uint32_t min_height = capabilities.minImageExtent.height > height 
            ? capabilities.minImageExtent.height 
            : height;
        extent->height = capabilities.maxImageExtent.height < min_height 
            ? capabilities.maxImageExtent.height 
            : min_height; 
    } 
    else {
        *extent = capabilities.currentExtent; 
    }

    return false;
}

bool CreateVulkanSwapchain(
    const VulkanDevice              device                  [static 1], 
    const VulkanSwapchainCreateInfo create_info             [static 1], 
    VulkanSwapchain                 swapchain               [static 1]
) {
    assert(create_info->render_pass != NULL);

    if (SelectVkPresentModeKHR(device, &swapchain->present_mode)) {
        ROSINA_LOG_ERROR("Could not select present mode");
        return true;
    }

    if (SelectSwapchainExtent(device, create_info->width, create_info->height, &swapchain->extent)) {
        ROSINA_LOG_ERROR("Could not select extent");
        return true;
    }
    
    VkSurfaceCapabilitiesKHR capabilities = {};
    VK_ERROR_RETURN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical_device, device->surface, &capabilities), true);

    const VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = device->surface,
        .minImageCount = capabilities.minImageCount,
        .imageFormat = create_info->render_pass->surface_format.format,
        .imageColorSpace = create_info->render_pass->surface_format.colorSpace,
        .imageExtent = swapchain->extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = create_info->queue_family_index_count,
        .pQueueFamilyIndices = create_info->queue_family_indices,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = swapchain->present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = swapchain->handle // VK_NULL_HANDLE ?
    };

    VK_ERROR_RETURN(vkCreateSwapchainKHR(device->handle, &swapchain_create_info, NULL, &swapchain->handle), true);
    return false;
}

