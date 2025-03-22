#include <assert.h>
#include <engine/backend/vulkan_helpers.h>
#include <stdlib.h>
#include <string.h>

const char* string_VkResult(VkResult input_value)
{
    switch (input_value)
    {
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
            return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTATION:
            return "VK_ERROR_FRAGMENTATION";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
            return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_ERROR_NOT_PERMITTED_KHR:
            return "VK_ERROR_NOT_PERMITTED_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_UNKNOWN:
            return "VK_ERROR_UNKNOWN";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_OPERATION_DEFERRED_KHR:
            return "VK_OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR:
            return "VK_OPERATION_NOT_DEFERRED_KHR";
        case VK_PIPELINE_COMPILE_REQUIRED:
            return "VK_PIPELINE_COMPILE_REQUIRED";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_THREAD_DONE_KHR:
            return "VK_THREAD_DONE_KHR";
        case VK_THREAD_IDLE_KHR:
            return "VK_THREAD_IDLE_KHR";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        default:
            return "Unhandled VkResult";
    }
}

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

uint32_t FindQueueFamilyIndex(const VulkanDevice device[static 1], const FindQueueFamilyIndexInfo info[static 1])
{
    uint32_t queue_family_property_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device->physical_device, &queue_family_property_count, NULL);

    VkQueueFamilyProperties* const queue_family_properties = calloc(queue_family_property_count, sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device->physical_device, &queue_family_property_count, queue_family_properties);

    for (uint32_t i = 0; i < queue_family_property_count; i++)
    {
        if ((info->flags & QUEUE_CAPABILITY_FLAG_GRAPHICS_BIT) && !queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            continue;
        }

        if ((info->flags & QUEUE_CAPABILITY_FLAG_TRANSFER_BIT) && !(queue_family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT))
        {
            continue;
        }

        if ((info->flags & QUEUE_CAPABILITY_FLAG_COMPUTE_BIT) && !(queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
        {
            continue;
        }

        VkBool32 surface_supported = VK_FALSE;
        VK_ERROR_HANDLE(vkGetPhysicalDeviceSurfaceSupportKHR(device->physical_device, i, info->surface, &surface_supported), { return UINT32_MAX; });
        if ((info->flags & QUEUE_CAPABILITY_FLAG_PRESENT_BIT) && surface_supported != VK_TRUE)
        {
            continue;
        }

        free(queue_family_properties);
        return i;
    }

    free(queue_family_properties);
    return UINT32_MAX;
}

void VulkanDevice_Cleanup(VulkanDevice device[static 1])
{
    vkDestroyDevice(device->handle, NULL);
    device->handle = VK_NULL_HANDLE;
}

static inline uint32_t RankPhysicalDevice(const VkSurfaceKHR surface, const VkPhysicalDevice physical_device)
{
    VkPhysicalDeviceProperties2 physical_device_properties = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = NULL, .properties = {}};
    vkGetPhysicalDeviceProperties2(physical_device, &physical_device_properties);

    if (physical_device_properties.properties.apiVersion < VK_MAKE_API_VERSION(0, 1, 3, 0))
    {
        return 0;
    }

    switch (physical_device_properties.properties.deviceType)
    {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            return 0;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return 2;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return 4;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return 3;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return 1;
        case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM:
            return 0;
        default:
            return 0;
    }
}

static inline bool SelectVkPhysicalDevice(const VkInstance instance, const VkSurfaceKHR surface, VkPhysicalDevice physical_device[static 1])
{
    uint32_t physical_device_count = 0;
    VK_ERROR_RETURN(vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL), true);
    VkPhysicalDevice* const physical_devices = calloc(physical_device_count, sizeof(VkPhysicalDevice));
    VK_ERROR_RETURN(vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices), true);

    uint32_t best_physical_device_index = 0;
    uint32_t best_physical_device_score = 0;
    for (uint32_t i = 0; i < physical_device_count; i++)
    {
        const uint32_t score = RankPhysicalDevice(surface, physical_devices[i]);
        if (score > best_physical_device_score)
        {
            best_physical_device_score = score;
            best_physical_device_index = i;
        }
    }

    if (best_physical_device_score == 0)
    {
        free(physical_devices);
        return true;
    }

    *physical_device = physical_devices[best_physical_device_index];
    free(physical_devices);
    return false;
}

bool CalculateVkDeviceQueueCreateInfo(const float* const priority, uint32_t queue_count, VkDeviceQueueCreateInfo* queue_create_infos)
{
    for (uint32_t i = 0; i < queue_count; i++)
    {
        const VkDeviceQueueCreateInfo queue_create_info = {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext            = NULL,
            .flags            = 0,
            .queueFamilyIndex = 0,
            .queueCount       = 1,
            .pQueuePriorities = priority,
        };
        queue_create_infos[i] = queue_create_info;
    }

    return true;
}

static inline bool CreateVkDevice(const VkPhysicalDevice physical_device, uint32_t queue_index_count, uint32_t* queue_indices, VkDevice device[static 1])
{
    uint32_t byte_count = 0;
    byte_count += (sizeof(VkDeviceQueueCreateInfo) * queue_index_count);  // queue_create_infos
    byte_count += (sizeof(char*) * 1);                                    // VK_KHR_SWAPCHAIN_EXTENSION_NAME

    void* const bytes = calloc(byte_count, 1);
    byte_count        = 0;

    const float priority                              = 1.0f;
    VkDeviceQueueCreateInfo* const queue_create_infos = bytes + byte_count;
    byte_count += (sizeof(VkDeviceQueueCreateInfo) * queue_index_count);
    for (uint32_t i = 0; i < queue_index_count; i++)
    {
        const VkDeviceQueueCreateInfo create_info = {.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                     .pNext            = NULL,
                                                     .flags            = 0,
                                                     .queueFamilyIndex = queue_indices[i],
                                                     .queueCount       = 1,
                                                     .pQueuePriorities = &priority};
        queue_create_infos[i]                     = create_info;
    }

    VkPhysicalDeviceFeatures2 physical_device_features = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = NULL, .features = {}};
    vkGetPhysicalDeviceFeatures2(physical_device, &physical_device_features);
    const char** enabled_extensions = bytes + byte_count;
    byte_count += (sizeof(char*) * 1);
    enabled_extensions[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

    const VkDeviceCreateInfo create_info = {.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                            .pNext                   = NULL,
                                            .flags                   = 0,
                                            .queueCreateInfoCount    = queue_index_count,
                                            .pQueueCreateInfos       = queue_create_infos,
                                            .enabledLayerCount       = 0,
                                            .ppEnabledLayerNames     = NULL,
                                            .enabledExtensionCount   = 1,
                                            .ppEnabledExtensionNames = enabled_extensions,
                                            .pEnabledFeatures        = &physical_device_features.features};

    VK_ERROR_HANDLE(vkCreateDevice(physical_device, &create_info, NULL, device), {
        free(bytes);
        return true;
    })

    free(bytes);
    return false;
}

int cmp_uint32_t(const void* x, const void* y)
{
    const uint32_t a = *((uint32_t*)x);
    const uint32_t b = *((uint32_t*)y);

    if (a > b) return 1;
    if (a < b) return -1;

    return 0;
}

bool CreateVulkanDevice(const VulkanDeviceCreateInfo create_info[static 1], VulkanDevice device[static 1])
{
    if (SelectVkPhysicalDevice(create_info->instance, create_info->surface, &device->physical_device))
    {
        ROSINA_LOG_ERROR("Could not select VkPhysicalDevice");
        VulkanDevice_Cleanup(device);
        return true;
    }

    uint32_t queue_family_index_count    = 0;
    uint32_t* const queue_family_indices = calloc(3, sizeof(uint32_t));
    {
        FindQueueFamilyIndexInfo find_info = {.flags = QUEUE_CAPABILITY_FLAG_GRAPHICS_BIT, .queue_count = 1, .surface = create_info->surface};

        device->graphics_queue.family_index = FindQueueFamilyIndex(device, &find_info);
        if (device->graphics_queue.family_index == UINT32_MAX)
        {
            free(queue_family_indices);
            return true;
        }
        device->graphics_queue.queue_index               = 0;
        queue_family_indices[queue_family_index_count++] = device->graphics_queue.family_index;

        find_info.flags                     = QUEUE_CAPABILITY_FLAG_TRANSFER_BIT;
        device->transfer_queue.family_index = FindQueueFamilyIndex(device, &find_info);
        if (device->transfer_queue.family_index == UINT32_MAX)
        {
            free(queue_family_indices);
            return true;
        }
        device->transfer_queue.queue_index               = 0;
        queue_family_indices[queue_family_index_count++] = device->transfer_queue.family_index;

        find_info.flags                    = QUEUE_CAPABILITY_FLAG_PRESENT_BIT;
        device->present_queue.family_index = FindQueueFamilyIndex(device, &find_info);
        if (device->present_queue.family_index == UINT32_MAX)
        {
            free(queue_family_indices);
            return true;
        }
        device->present_queue.queue_index                = 0;
        queue_family_indices[queue_family_index_count++] = device->present_queue.family_index;

        // sort
        qsort(queue_family_indices, queue_family_index_count, sizeof(uint32_t), cmp_uint32_t);

        // remove duplicates
        {
            uint32_t l = 0;
            uint32_t m = 0;
            uint32_t r = 0;
            while (r < queue_family_index_count)
            {
                while (r < queue_family_index_count && queue_family_indices[m] == queue_family_indices[r]) r++;

                queue_family_indices[l++] = queue_family_indices[m];
                m                         = r;
            }
            queue_family_index_count = l;
        }
    }

    if (CreateVkDevice(device->physical_device, queue_family_index_count, queue_family_indices, &device->handle))
    {
        ROSINA_LOG_ERROR("Could not create VkDevice");
        free(queue_family_indices);
        VulkanDevice_Cleanup(device);
        return true;
    }

    {
        VkDeviceQueueInfo2 queue_info = {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
            .pNext            = NULL,
            .flags            = 0,
            .queueFamilyIndex = device->graphics_queue.family_index,
            .queueIndex       = device->graphics_queue.queue_index,
        };
        vkGetDeviceQueue2(device->handle, &queue_info, &device->graphics_queue.handle);

        queue_info.queueFamilyIndex = device->transfer_queue.family_index;
        queue_info.queueIndex       = device->transfer_queue.queue_index;
        vkGetDeviceQueue2(device->handle, &queue_info, &device->transfer_queue.handle);

        queue_info.queueFamilyIndex = device->present_queue.family_index;
        queue_info.queueIndex       = device->present_queue.queue_index;
        vkGetDeviceQueue2(device->handle, &queue_info, &device->present_queue.handle);
    }

    free(queue_family_indices);
    return false;
}

void VulkanSwapchain_Cleanup(const VulkanDevice device[static 1], VulkanSwapchain swapchain[static 1])
{
    vkDestroySwapchainKHR(device->handle, swapchain->handle, NULL);
    swapchain->handle = VK_NULL_HANDLE;
}

bool VulkanSwapchain_Create(const VulkanDevice device[static 1], const VulkanSwapchainCreateInfo create_info[static 1], VulkanSwapchain swapchain[static 1])
{
    assert(create_info->render_pass != NULL);
    assert(swapchain->present_mode == VK_PRESENT_MODE_FIFO_KHR);

    {
        uint32_t supported_mode_count = 0;
        VK_ERROR_RETURN(vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical_device, create_info->surface, &supported_mode_count, NULL), true);
        VkPresentModeKHR* const supported_modes = calloc(supported_mode_count, sizeof(VkPresentModeKHR));
        VK_ERROR_HANDLE(vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical_device, create_info->surface, &supported_mode_count, supported_modes), {
            free(supported_modes);
            return true;
        });

        for (uint32_t i = 0; i < supported_mode_count; i++)
        {
            if (supported_modes[i] != VK_PRESENT_MODE_MAILBOX_KHR)
            {
                continue;
            }
            swapchain->present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }

        free(supported_modes);
    }

    {
        VkSurfaceCapabilitiesKHR capabilities = {};
        VK_ERROR_RETURN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical_device, create_info->surface, &capabilities), true);

        // Here is an example of how to do this differently?
        // https://github.com/LunarG/VulkanSamples/blob/master/API-Samples/05-init_swapchain/05-init_swapchain.cpp#L170
        if (capabilities.currentExtent.width == 0xFFFFFFFF)
        {
            swapchain->extent.width = create_info->width;
            swapchain->extent.height = create_info->height;

            if (swapchain->extent.width < capabilities.minImageExtent.width) {
                swapchain->extent.width = capabilities.minImageExtent.width;
            } else if (swapchain->extent.width > capabilities.maxImageExtent.width) {
                swapchain->extent.width = capabilities.maxImageExtent.width;
            }

            if (swapchain->extent.height < capabilities.minImageExtent.height) {
                swapchain->extent.height = capabilities.minImageExtent.height;
            } else if (swapchain->extent.height > capabilities.maxImageExtent.height) {
                swapchain->extent.height = capabilities.maxImageExtent.height;
            }
        }
        else
        {
            swapchain->extent = capabilities.currentExtent;
        }
    }

    VkSurfaceCapabilitiesKHR capabilities = {};
    VK_ERROR_RETURN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical_device, create_info->surface, &capabilities), true);

    const VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext                 = NULL,
        .flags                 = 0,
        .surface               = create_info->surface,
        .minImageCount         = capabilities.minImageCount,
        .imageFormat           = create_info->render_pass->surface_format.format,
        .imageColorSpace       = create_info->render_pass->surface_format.colorSpace,
        .imageExtent           = swapchain->extent,
        .imageArrayLayers      = 1,
        .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = NULL,
        .preTransform          = capabilities.currentTransform,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = swapchain->present_mode,
        .clipped               = VK_TRUE,
        .oldSwapchain          = swapchain->handle  // VK_NULL_HANDLE ?
    };

    VK_ERROR_RETURN(vkCreateSwapchainKHR(device->handle, &swapchain_create_info, NULL, &swapchain->handle), true);
    return false;
}
