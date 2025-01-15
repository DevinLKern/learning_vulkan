#include <engine/backend/glfw_helpers.h>

#include <utility/log.h>
#include <assert.h>

bool glfw_initialized = false;

void UninitializeGraphicsLibraryFramework() {
    assert(glfw_initialized);
    glfwTerminate();
}

bool InitializeGraphicsLibraryFramework() {
    assert(!glfw_initialized);

    if (glfwInit() == GLFW_FALSE) return true;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    glfw_initialized = true;
    return false;
}

bool GetRequiredGLFWExtensions(uint32_t extension_count [static 1], const char** extension_names) {
    assert(glfw_initialized);

    const char** names = glfwGetRequiredInstanceExtensions(extension_count);
    if (names == NULL) {
        ROSINA_LOG_ERROR("Could not get glfw extensions");
        return true;
    }
    if (extension_names == NULL) {
        return false;
    }

    for (uint32_t i = 0; i < *extension_count; i++) {
        extension_names[i] = names[i];
    }

    return false;
}

void GLFWWindow_Cleanup(GLFWWindow window [static 1]) {
    assert(glfw_initialized);

    glfwDestroyWindow(window->window);
}

GLFWWindow GLFWWindow_Create(const GLFWWindowCreateInfo create_info [static 1]) {
    assert(glfw_initialized);

    GLFWWindow window = {
        .window = NULL,
        .width = create_info->height,
        .height = create_info->width
    };

    window.window = glfwCreateWindow(create_info->width, create_info->height, create_info->title, NULL, NULL);
    if (window.window == NULL) {
        ROSINA_LOG_ERROR("Could not create glfw window");
        return window;
    }

    window.width = create_info->width;
    window.height = create_info->height;
    return window;
}