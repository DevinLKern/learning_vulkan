#include <assert.h>
#include <engine/backend/vulkan_helpers.h>
#include <stdlib.h>
#include <string.h>

void VulkanDevice_Cleanup(VulkanDevice device[static 1])
{
    while (device->component_count > 0)
    {
        switch (device->components[--device->component_count])
        {
            case VULKAN_DEVICE_COMPONENT_INSTANCE:
                vkDestroyInstance(device->instance, NULL);
                break;
            case VULKAN_DEVICE_COMPONENT_DEBUG_MESSENGER:
                vkDestroyDebugUtilsMessengerEXT(device->instance, device->debug_messenger, NULL);
                break;
            case VULKAN_DEVICE_COMPONENT_SURFACE:
                vkDestroySurfaceKHR(device->instance, device->surface, NULL);
                break;
            case VULKAN_DEVICE_COMPONENT_DEVICE:
                vkDestroyDevice(device->handle, NULL);
                break;
            default:
                ROSINA_LOG_ERROR("Invalid swapchain component value");
                assert(false);
        }
    }
}

static inline bool CreateVkInstance(VkInstance instance[static 1], const bool debug)
{
    uint32_t available_layer_count     = 0;
    uint32_t required_layer_count      = 0;
    uint32_t available_extension_count = 0;
    uint32_t required_extension_count  = 0;

    uint32_t byte_count = 0;
    // calculate the amount of memory needed in bytes
    {
        if (debug)
        {
            required_layer_count++;  // VK_LAYER_KHRONOS_validation
        }
        byte_count += (sizeof(char*) * required_layer_count);

        VK_ERROR_RETURN(vkEnumerateInstanceLayerProperties(&available_layer_count, NULL), true);
        byte_count += (sizeof(VkLayerProperties) * available_layer_count);

        byte_count++;  // VK_KHR_get_surface_capabilities2
        if (debug)
        {
            byte_count++;  // VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        }
        byte_count += (sizeof(char*) * byte_count);

        VK_ERROR_RETURN(vkEnumerateInstanceExtensionProperties(NULL, &available_extension_count, NULL), true);
        byte_count += (sizeof(VkExtensionProperties) * available_extension_count);
    }

    void* const bytes = calloc(byte_count, 1);

    byte_count               = 0;
    required_layer_count     = 0;
    required_extension_count = 0;

    // use memory to do stuff
    const char** const required_layers = bytes + byte_count;
    {
        if (debug)
        {
            required_layers[required_layer_count++] = "VK_LAYER_KHRONOS_validation";
        }
        byte_count += (sizeof(char*) * required_layer_count);

        VkLayerProperties* const available_layers = bytes + byte_count;
        byte_count += (sizeof(VkLayerProperties) * available_layer_count);
        VK_ERROR_HANDLE(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers), {
            free(bytes);
            return true;
        });

        // find required layers in available layers
        for (uint32_t i = 0; i < required_layer_count; i++)
        {
            bool found = false;
            for (uint32_t j = 0; j < available_layer_count; j++)
            {
                if (strcmp(required_layers[i], available_layers[j].layerName) == 0)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                ROSINA_LOG_ERROR("Could not find required layer '%s'", required_layers[i]);
                free(bytes);
                return true;
            }
        }
    }

    const char** const required_extensions = bytes + byte_count;
    {
        if (GetRequiredGLFWExtensions(&required_extension_count, required_extensions))
        {
            ROSINA_LOG_ERROR("Could not get required extensions");
            free(bytes);
            return true;
        }
        required_extensions[required_extension_count++] = "VK_KHR_get_surface_capabilities2";
        if (debug)
        {
            required_extensions[required_extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        }
        byte_count += (sizeof(char*) * required_extension_count);

        VkExtensionProperties* const available_extensions = bytes + byte_count;
        byte_count += (sizeof(VkExtensionProperties) * available_extension_count);

        VK_ERROR_RETURN(vkEnumerateInstanceExtensionProperties(NULL, &available_extension_count, available_extensions), true);
        // find required extensions in available extensions
        for (uint32_t i = 0; i < required_extension_count; i++)
        {
            bool found = false;
            for (uint32_t j = 0; j < available_extension_count; j++)
            {
                if (strcmp(required_extensions[i], available_extensions[j].extensionName) == 0)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                ROSINA_LOG_ERROR("Could not find required extension '%s'", required_extensions[i]);
                free(bytes);
                return true;
            }
        }
    }

    // zero out patch bytes for improved compatibility
    const VkApplicationInfo app_info = {.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                        .pNext              = NULL,
                                        .pApplicationName   = "sandbox",
                                        .applicationVersion = 0,
                                        .pEngineName        = "rosina",
                                        .engineVersion      = 0,
                                        .apiVersion         = VK_MAKE_API_VERSION(0, 1, 3, 0)};

    {
        uint32_t supported_api_version = 0;
        VK_ERROR_HANDLE(vkEnumerateInstanceVersion(&supported_api_version), {
            free(bytes);
            return true;
        });

        ROSINA_LOG_INFO("Vulkan version: %d.%d is supported", VK_API_VERSION_MAJOR(supported_api_version), VK_API_VERSION_MINOR(supported_api_version));

        if (app_info.apiVersion > supported_api_version)
        {
            ROSINA_LOG_ERROR("Required api version not supported");
            return true;
        }
    }

    const VkInstanceCreateInfo create_info = {.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                              .pNext                   = NULL,
                                              .flags                   = 0,
                                              .pApplicationInfo        = &app_info,
                                              .enabledLayerCount       = required_layer_count,
                                              .ppEnabledLayerNames     = required_layers,
                                              .enabledExtensionCount   = required_extension_count,
                                              .ppEnabledExtensionNames = required_extensions};
    VK_ERROR_HANDLE(vkCreateInstance(&create_info, NULL, instance), {
        free(bytes);
        return true;
    });

    free(bytes);
    return false;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                             const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    ROSINA_LOG_INFO("%s", pCallbackData->pMessage);
    return VK_TRUE;
}

VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                                        VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func == NULL) return VK_ERROR_EXTENSION_NOT_PRESENT;
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
}

void vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != NULL) func(instance, debugMessenger, pAllocator);
}

bool CreateVkDebugUtilsMessengerEXT(const VkInstance instance, const VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT debug_messenger[static 1])
{
    const VkDebugUtilsMessengerCreateInfoEXT create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
        .pUserData       = NULL};

    VK_ERROR_RETURN(vkCreateDebugUtilsMessengerEXT(instance, &create_info, allocator, debug_messenger), true);
    return false;
}

static inline bool CreateVkSurfaceKHR(const VkInstance instance, const VkAllocationCallbacks* allocator, const GLFWWindow window[static 1],
                                      VkSurfaceKHR surface[static 1])
{
    VK_ERROR_RETURN(glfwCreateWindowSurface(instance, window->window, allocator, surface), true);
    return false;
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

bool CreateVulkanDevice(const VulkanDeviceCreateInfo create_info[static 1], VulkanDevice device[static 1])
{
    if (CreateVkInstance(&device->instance, create_info->debug))
    {
        ROSINA_LOG_ERROR("Could not create VkInstance");
        VulkanDevice_Cleanup(device);
        return true;
    }
    device->components[device->component_count++] = VULKAN_DEVICE_COMPONENT_INSTANCE;

    if (CreateVkDebugUtilsMessengerEXT(device->instance, NULL, &device->debug_messenger))
    {
        ROSINA_LOG_ERROR("Could not create DebugUtilsMessengerEXT");
        VulkanDevice_Cleanup(device);
        return true;
    }
    device->components[device->component_count++] = VULKAN_DEVICE_COMPONENT_DEBUG_MESSENGER;

    if (CreateVkSurfaceKHR(device->instance, NULL, create_info->window, &device->surface))
    {
        ROSINA_LOG_ERROR("Could not create VkSurfaceKHR");
        VulkanDevice_Cleanup(device);
        return true;
    }
    device->components[device->component_count++] = VULKAN_DEVICE_COMPONENT_SURFACE;

    if (SelectVkPhysicalDevice(device->instance, device->surface, &device->physical_device))
    {
        ROSINA_LOG_ERROR("Could not select VkPhysicalDevice");
        VulkanDevice_Cleanup(device);
        return true;
    }

    const FindQueueFamilyIndexInfo find_info = {.queue_capability_count = create_info->queue_description_count,
                                                .queue_capabilities     = create_info->queue_descriptions};
    uint32_t* const queue_family_indices     = calloc(create_info->queue_description_count, sizeof(uint32_t));
    for (uint32_t i = 0; i < create_info->queue_description_count; i++)
    {
        queue_family_indices[i] = UINT32_MAX;
    }
    if (FindQueueFamilyIndices(device, &find_info, queue_family_indices))
    {
        ROSINA_LOG_ERROR("Queue family not supported");
        free(queue_family_indices);
        VulkanDevice_Cleanup(device);
        return true;
    }

    if (CreateVkDevice(device->physical_device, create_info->queue_description_count, queue_family_indices, &device->handle))
    {
        ROSINA_LOG_ERROR("Could not create VkDevice");
        free(queue_family_indices);
        VulkanDevice_Cleanup(device);
        return true;
    }
    device->components[device->component_count++] = VULKAN_DEVICE_COMPONENT_DEVICE;

    free(queue_family_indices);
    return false;
}
