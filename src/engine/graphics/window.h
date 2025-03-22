#ifndef WINDOW_H
#define WINDOW_H

#include <engine/event.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdbool.h>

typedef struct GraphicsLink
{
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
} GraphicsLink;

void GraphicsLink_Cleanup(GraphicsLink link[static 1]);

GraphicsLink GraphicsLink_Create(const bool debug);

typedef struct Window
{
    GLFWwindow* handle;
    uint32_t width;
    uint32_t height;
    VkSurfaceKHR surface;
} Window;

void Window_Cleanup(const GraphicsLink link[static 1], Window window[static 1]);

typedef struct WindowCreateInfo
{
    uint32_t width;
    uint32_t height;
    const char* title;
    const GraphicsLink* link;
} WindowCreateInfo;

Window Window_Create(const WindowCreateInfo create_info[static 1]);

void Window_SetKeyboardEventCallbackFunction(const Window window[static 1], const EventHandler handler);

void Window_SetMouseEventCallbackFunction(const Window window[static 1], const EventHandler handler);

static inline bool Window_ShouldClose(const Window window[static 1])
{
    return (bool)glfwWindowShouldClose(window->handle);
}

static inline void Window_PollEvents(const Window window[static 1])
{
    glfwPollEvents();
}

#endif //WINDOW_H
