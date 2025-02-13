#include <engine/graphics/graphics.h>
#include <string.h>

bool GetRequiredGLFWExtensions(uint32_t extension_count[static 1], const char** extension_names)
{
    const char** names = glfwGetRequiredInstanceExtensions(extension_count);
    if (names == NULL)
    {
        ROSINA_LOG_ERROR("Could not get glfw extensions");
        return true;
    }
    if (extension_names == NULL)
    {
        return false;
    }

    for (uint32_t i = 0; i < *extension_count; i++)
    {
        extension_names[i] = names[i];
    }

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

GraphicsLink GraphicsLink_Create(const bool debug)
{
    GraphicsLink link = {.instance = VK_NULL_HANDLE, .debug_messenger = VK_NULL_HANDLE};

    if (glfwInit() == GLFW_FALSE)
    {
        return link;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // instance
    {
        uint32_t available_layer_count     = 0;
        uint32_t required_layer_count      = 0;
        uint32_t available_extension_count = 0;
        uint32_t required_extension_count  = 0;

        // calculate the amount of memory needed in bytes
        MemoryArena arena;
        {
            VK_ERROR_HANDLE(vkEnumerateInstanceLayerProperties(&available_layer_count, NULL), {
                glfwTerminate();
                return link;
            });

            if (debug) required_layer_count++;  // VK_LAYER_KHRONOS_validation

            VK_ERROR_HANDLE(vkEnumerateInstanceExtensionProperties(NULL, &available_extension_count, NULL), {
                glfwTerminate();
                return link;
            });

            if (GetRequiredGLFWExtensions(&required_extension_count, NULL))
            {
                glfwTerminate();
                return link;
            }
            required_extension_count++;             // VK_KHR_get_surface_capabilities2
            if (debug) required_extension_count++;  // VK_EXT_DEBUG_UTILS_EXTENSION_NAME

            arena = MemoryArena_Create((sizeof(char*) * required_layer_count) + (sizeof(char*) * required_extension_count) +
                                       (sizeof(VkLayerProperties) * available_layer_count) + (sizeof(VkExtensionProperties) * available_extension_count));
        }

        // use memory to do stuff
        VkLayerProperties* const available_layers         = MemoryArena_Allocate(&arena, available_layer_count * sizeof(VkLayerProperties));
        const char** const required_layers                = MemoryArena_Allocate(&arena, required_layer_count * sizeof(char*));
        VkExtensionProperties* const available_extensions = MemoryArena_Allocate(&arena, available_extension_count * sizeof(VkExtensionProperties));
        const char** const required_extensions            = MemoryArena_Allocate(&arena, required_extension_count * sizeof(char*));
        required_layer_count                              = 0;
        required_extension_count                          = 0;
        {
            VK_ERROR_HANDLE(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers), {
                MemoryArena_Free(&arena);
                glfwTerminate();
                return link;
            });

            if (debug) required_layers[required_layer_count++] = "VK_LAYER_KHRONOS_validation";

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
                    ROSINA_LOG_ERROR("Could not find layer named \"%s\"\n", required_layers[i]);
                    MemoryArena_Free(&arena);
                    glfwTerminate();
                    return link;
                }
            }
        }
        {
            VK_ERROR_HANDLE(vkEnumerateInstanceExtensionProperties(NULL, &available_extension_count, available_extensions), {
                MemoryArena_Free(&arena);
                glfwTerminate();
                return link;
            });

            if (GetRequiredGLFWExtensions(&required_extension_count, required_extensions))
            {
                MemoryArena_Free(&arena);
                glfwTerminate();
                return link;
            }
            required_extensions[required_extension_count++] = "VK_KHR_get_surface_capabilities2";
            if (debug) required_extensions[required_extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

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
                    ROSINA_LOG_ERROR("Could not find extension named \"%s\"\n", required_extensions[i]);
                    MemoryArena_Free(&arena);
                    glfwTerminate();
                    return link;
                }
            }
        }

        {
            // zero out patch bytes for improved compatibility
            const VkApplicationInfo app_info = {.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                                .pNext              = NULL,
                                                .pApplicationName   = "sandbox",
                                                .applicationVersion = 0,
                                                .pEngineName        = "rosina",
                                                .engineVersion      = 0,
                                                .apiVersion         = VK_MAKE_API_VERSION(0, 1, 3, 0)};

            uint32_t supported_api_version = 0;
            VK_ERROR_HANDLE(vkEnumerateInstanceVersion(&supported_api_version), {
                MemoryArena_Free(&arena);
                glfwTerminate();
                return link;
            });

            ROSINA_LOG_INFO("Vulkan version: %d.%d is supported", VK_API_VERSION_MAJOR(supported_api_version), VK_API_VERSION_MINOR(supported_api_version));

            if (app_info.apiVersion > supported_api_version)
            {
                ROSINA_LOG_ERROR("Required api version not supported");
                MemoryArena_Free(&arena);
                glfwTerminate();
                return link;
            }

            const VkInstanceCreateInfo create_info = {.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                                      .pNext                   = NULL,
                                                      .flags                   = 0,
                                                      .pApplicationInfo        = &app_info,
                                                      .enabledLayerCount       = required_layer_count,
                                                      .ppEnabledLayerNames     = required_layers,
                                                      .enabledExtensionCount   = required_extension_count,
                                                      .ppEnabledExtensionNames = required_extensions};
            VK_ERROR_HANDLE(vkCreateInstance(&create_info, NULL, &link.instance), {
                MemoryArena_Free(&arena);
                glfwTerminate();
                return link;
            });

            MemoryArena_Free(&arena);
        }
    }

    // debug messenger
    {
        const VkDebugUtilsMessengerCreateInfoEXT create_info = {
            .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debugCallback,
            .pUserData       = NULL};

        VK_ERROR_HANDLE(vkCreateDebugUtilsMessengerEXT(link.instance, &create_info, NULL, &link.debug_messenger), {
            vkDestroyInstance(link.instance, NULL);
            link.instance = VK_NULL_HANDLE;
            glfwTerminate();
            return link;
        });
    }

    return link;
}

void GraphicsLink_Cleanup(GraphicsLink link[static 1])
{
    vkDestroyDebugUtilsMessengerEXT(link->instance, link->debug_messenger, NULL);
    link->debug_messenger = VK_NULL_HANDLE;
    vkDestroyInstance(link->instance, NULL);
    link->instance = VK_NULL_HANDLE;
    glfwTerminate();
}

void Window_Cleanup(const GraphicsLink link[static 1], Window window[static 1])
{
    vkDestroySurfaceKHR(link->instance, window->surface, NULL);
    window->surface = VK_NULL_HANDLE;
    glfwDestroyWindow(window->handle);
    window->handle = NULL;
}

Window Window_Create(const WindowCreateInfo create_info[static 1])
{
    Window window = {.handle = NULL, .width = UINT32_MAX, .height = UINT32_MAX};

    window.handle = glfwCreateWindow(create_info->width, create_info->height, create_info->title, NULL, NULL);

    window.width  = create_info->width;
    window.height = create_info->height;

    VK_ERROR_HANDLE(glfwCreateWindowSurface(create_info->link->instance, window.handle, NULL, &window.surface), {
        glfwDestroyWindow(window.handle);
        window.handle = NULL;
        return window;
    });

    return window;
}