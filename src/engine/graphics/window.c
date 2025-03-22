#include <engine/graphics/window.h>

#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include <engine/backend/vulkan_helpers.h>

#include <utility/memory_arena.h>

void GraphicsLink_Cleanup(GraphicsLink link[static 1])
{
    vkDestroyDebugUtilsMessengerEXT(link->instance, link->debug_messenger, NULL);
    link->debug_messenger = VK_NULL_HANDLE;
    vkDestroyInstance(link->instance, NULL);
    link->instance = VK_NULL_HANDLE;

    glfwTerminate();
}

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

void Window_Cleanup(const GraphicsLink link[static 1], Window window[static 1])
{
    assert(window->surface != VK_NULL_HANDLE);
    assert(window->handle != NULL);

    vkDestroySurfaceKHR(link->instance, window->surface, NULL);
    window->surface = VK_NULL_HANDLE;
    glfwDestroyWindow(window->handle);
    window->handle = NULL;

}

Window Window_Create(const WindowCreateInfo create_info[static 1])
{
    Window window = {.handle = NULL, .width = UINT32_MAX, .height = UINT32_MAX};

    window.handle = glfwCreateWindow((int)create_info->width, (int)create_info->height, create_info->title, NULL, NULL);
    if (window.handle == NULL)
    {
        ROSINA_LOG_ERROR("Failed to create window");
        return window;
    }

    window.width  = create_info->width;
    window.height = create_info->height;

    VK_ERROR_HANDLE(glfwCreateWindowSurface(create_info->link->instance, window.handle, NULL, &window.surface), {
        glfwDestroyWindow(window.handle);
        window.handle = NULL;
        return window;
    });

    return window;
}

EventType MapGLFWActionToEventType(const int action)
{
    switch (action)
    {
        case GLFW_RELEASE:
            return EVENT_TYPE_RELEASE;
        case GLFW_PRESS:
            return EVENT_TYPE_PRESS;
        case GLFW_REPEAT:
            return EVENT_TYPE_REPEAT;
        default:
            ROSINA_LOG_ERROR("Error. Invalid action value");
            assert(false);
    }
}

KeyboardKey MapGLFWKeyToKeyboardKey(const int key)
{
    switch (key)
    {
        case GLFW_KEY_SPACE:
            return KEYBORAD_KEY_SPACE;
        case GLFW_KEY_APOSTROPHE:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_COMMA:
            return KEYBORAD_KEY_COMMA;
        case GLFW_KEY_MINUS:
            return KEYBORAD_KEY_MINUS;
        case GLFW_KEY_PERIOD:
            return KEYBORAD_KEY_PERIOD;
        case GLFW_KEY_SLASH:
            return KEYBORAD_KEY_SLASH;
        case GLFW_KEY_0:
            return KEYBORAD_KEY_0;
        case GLFW_KEY_1:
            return KEYBORAD_KEY_1;
        case GLFW_KEY_2:
            return KEYBORAD_KEY_2;
        case GLFW_KEY_3:
            return KEYBORAD_KEY_3;
        case GLFW_KEY_4:
            return KEYBORAD_KEY_4;
        case GLFW_KEY_5:
            return KEYBORAD_KEY_5;
        case GLFW_KEY_6:
            return KEYBORAD_KEY_6;
        case GLFW_KEY_7:
            return KEYBORAD_KEY_7;
        case GLFW_KEY_8:
            return KEYBORAD_KEY_8;
        case GLFW_KEY_9:
            return KEYBORAD_KEY_9;
        case GLFW_KEY_SEMICOLON:  // ----------
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_EQUAL:  // ----------
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_A:
            return KEYBORAD_KEY_A;
        case GLFW_KEY_B:
            return KEYBORAD_KEY_B;
        case GLFW_KEY_C:
            return KEYBORAD_KEY_C;
        case GLFW_KEY_D:
            return KEYBORAD_KEY_D;
        case GLFW_KEY_E:
            return KEYBORAD_KEY_E;
        case GLFW_KEY_F:
            return KEYBORAD_KEY_F;
        case GLFW_KEY_G:
            return KEYBORAD_KEY_G;
        case GLFW_KEY_H:
            return KEYBORAD_KEY_H;
        case GLFW_KEY_I:
            return KEYBORAD_KEY_I;
        case GLFW_KEY_J:
            return KEYBORAD_KEY_J;
        case GLFW_KEY_K:
            return KEYBORAD_KEY_K;
        case GLFW_KEY_L:
            return KEYBORAD_KEY_L;
        case GLFW_KEY_M:
            return KEYBORAD_KEY_M;
        case GLFW_KEY_N:
            return KEYBORAD_KEY_N;
        case GLFW_KEY_O:
            return KEYBORAD_KEY_O;
        case GLFW_KEY_P:
            return KEYBORAD_KEY_P;
        case GLFW_KEY_Q:
            return KEYBORAD_KEY_Q;
        case GLFW_KEY_R:
            return KEYBORAD_KEY_R;
        case GLFW_KEY_S:
            return KEYBORAD_KEY_S;
        case GLFW_KEY_T:
            return KEYBORAD_KEY_T;
        case GLFW_KEY_U:
            return KEYBORAD_KEY_U;
        case GLFW_KEY_V:
            return KEYBORAD_KEY_V;
        case GLFW_KEY_W:
            return KEYBORAD_KEY_W;
        case GLFW_KEY_X:
            return KEYBORAD_KEY_X;
        case GLFW_KEY_Y:
            return KEYBORAD_KEY_Y;
        case GLFW_KEY_Z:
            return KEYBORAD_KEY_Z;
        case GLFW_KEY_LEFT_BRACKET:
            return KEYBORAD_KEY_BRACKET_OPEN;
        case GLFW_KEY_BACKSLASH:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_RIGHT_BRACKET:
            return KEYBORAD_KEY_BRACKET_CLOSE;
        case GLFW_KEY_GRAVE_ACCENT:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_WORLD_1:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_WORLD_2:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_ESCAPE:
            return KEYBORAD_KEY_ESCAPE;
        case GLFW_KEY_ENTER:
            return KEYBORAD_KEY_ENTER;
        case GLFW_KEY_TAB:
            return KEYBORAD_KEY_TAB;
        case GLFW_KEY_BACKSPACE:
            return KEYBORAD_KEY_BACKSPACE;
        case GLFW_KEY_INSERT:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_DELETE:
            return KEYBORAD_KEY_DELETE;
        case GLFW_KEY_RIGHT:
            return KEYBORAD_KEY_RIGHT_ARROW;
        case GLFW_KEY_LEFT:
            return KEYBORAD_KEY_LEFT_ARROW;
        case GLFW_KEY_DOWN:
            return KEYBORAD_KEY_DOWN_ARROW;
        case GLFW_KEY_UP:
            return KEYBORAD_KEY_UP_ARROW;
        case GLFW_KEY_PAGE_UP:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_PAGE_DOWN:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_HOME:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_END:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_CAPS_LOCK:
            return KEYBORAD_KEY_CAPS;
        case GLFW_KEY_SCROLL_LOCK:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_NUM_LOCK:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_PRINT_SCREEN:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_PAUSE:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_F1:
            return KEYBORAD_KEY_F1;
        case GLFW_KEY_F2:
            return KEYBORAD_KEY_F2;
        case GLFW_KEY_F3:
            return KEYBORAD_KEY_F3;
        case GLFW_KEY_F4:
            return KEYBORAD_KEY_F4;
        case GLFW_KEY_F5:
            return KEYBORAD_KEY_F5;
        case GLFW_KEY_F6:
            return KEYBORAD_KEY_F6;
        case GLFW_KEY_F7:
            return KEYBORAD_KEY_F7;
        case GLFW_KEY_F8:
            return KEYBORAD_KEY_F8;
        case GLFW_KEY_F9:
            return KEYBORAD_KEY_F9;
        case GLFW_KEY_F10:
            return KEYBORAD_KEY_F10;
        case GLFW_KEY_F11:
            return KEYBORAD_KEY_F11;
        case GLFW_KEY_F12:
            return KEYBORAD_KEY_F12;
        case GLFW_KEY_F13:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_F14:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_F15:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_F16:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_F17:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_F18:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_F19:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_F20:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_F21:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_F22:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_F23:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_F24:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_F25:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_KP_0:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_KP_1:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_KP_2:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_KP_3:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_KP_4:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_KP_5:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_KP_6:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_KP_7:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_KP_8:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_KP_9:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_KP_DECIMAL:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_KP_DIVIDE:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_KP_MULTIPLY:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_KP_SUBTRACT:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_KP_ADD:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_KP_ENTER:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_KP_EQUAL:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_LEFT_SHIFT:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_LEFT_CONTROL:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_LEFT_ALT:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_LEFT_SUPER:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_RIGHT_SHIFT:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_RIGHT_CONTROL:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_RIGHT_ALT:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_RIGHT_SUPER:
            return KEYBORAD_KEY_NONE;
        case GLFW_KEY_MENU:
            return KEYBORAD_KEY_NONE;
        default:
            ROSINA_LOG_ERROR("Error. Invalid key value");
            assert(false);
    }
}
EventHandler keyboard_handle_fn = NULL;
void HandleGLFWKeyboardEvent(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    const Event e = {.keyboard_key = MapGLFWKeyToKeyboardKey(key), .type = MapGLFWActionToEventType(action)};
    keyboard_handle_fn(e);
}
void Window_SetKeyboardEventCallbackFunction(const Window window[static 1], const EventHandler handler)
{
    keyboard_handle_fn = handler;
    glfwSetKeyCallback(window->handle, HandleGLFWKeyboardEvent);
}

MouseButton MapGLFWButtonToMouseButton(const int button)
{
    switch (button)
    {
        case GLFW_MOUSE_BUTTON_1:
            return MOUSE_BUTTON_1;
        case GLFW_MOUSE_BUTTON_2:
            return MOUSE_BUTTON_2;
        case GLFW_MOUSE_BUTTON_3:
            return MOUSE_BUTTON_3;
        case GLFW_MOUSE_BUTTON_4:
            return MOUSE_BUTTON_4;
        case GLFW_MOUSE_BUTTON_5:
            return MOUSE_BUTTON_5;
        case GLFW_MOUSE_BUTTON_6:
            return MOUSE_BUTTON_6;
        case GLFW_MOUSE_BUTTON_7:
            return MOUSE_BUTTON_7;
        case GLFW_MOUSE_BUTTON_8:
            return MOUSE_BUTTON_8;
        default:
            ROSINA_LOG_ERROR("Error. Invalid key value");
            assert(false);
    }
}
EventHandler mouse_button_handle_fn = NULL;
void _HandleMouseButtonboardEvent(GLFWwindow* window, int button, int action, int mods)
{
    const Event e = {.mouse_button = MapGLFWButtonToMouseButton(button), .type = MapGLFWActionToEventType(action)};
    mouse_button_handle_fn(e);
}
void Window_SetMouseEventCallbackFunction(const Window window[static 1], const EventHandler handler)
{
    mouse_button_handle_fn = handler;
    glfwSetMouseButtonCallback(window->handle, _HandleMouseButtonboardEvent);
}

//