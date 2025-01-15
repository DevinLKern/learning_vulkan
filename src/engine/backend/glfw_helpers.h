#ifndef ROSINA_ENGINE_BACKEND_GLFW_H
#define ROSINA_ENGINE_BACKEND_GLFW_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdbool.h>

void UninitializeGraphicsLibraryFramework();

bool InitializeGraphicsLibraryFramework();

/**
 * Usage: 
 * extension_count cannot be NULL.
 * Sets extension_count to the required number of extensions when extension_names is NULL.
 * If extension_names is not NULL, it is populated with extension_count pointers to the names of the required extensions.
 */
bool GetRequiredGLFWExtensions(uint32_t extension_count [static 1], const char** extension_names);

typedef struct GLFWWindow {
    GLFWwindow* window;
    uint32_t    width;
    uint32_t    height;
} GLFWWindow;

void GLFWWindow_Cleanup(GLFWWindow window [static 1]);

typedef struct GLFWWindowCreateInfo {
    uint32_t    width;
    uint32_t    height;
    const char* title;
} GLFWWindowCreateInfo;

GLFWWindow GLFWWindow_Create(const GLFWWindowCreateInfo create_info [static 1]);


#endif