#ifndef ROSINA_ENGINE_FRONTEND_RENDERER_H
#define ROSINA_ENGINE_FRONTEND_RENDERER_H

#include <engine/backend/glfw_helpers.h>
#include <engine/backend/vulkan_helpers.h>

#include <utility/memory_arena.h>

typedef enum RendererContextComponent {
    RENDER_CONTEXT_DEVICE_COMPONENT,
    RENDER_CONTEXT_RENDER_PASS_COMPONENT,
    RENDER_CONTEXT_GRAPHICS_PIPELINE_COMPONENT,
    RENDER_CONTEXT_SWAPCHAIN_COMPONENT,
    RENDER_CONTEXT_DEPTH_IMAGES_COMPONENT,
    RENDER_CONTEXT_DEPTH_IMAGE_MEMORIES_COMPONENT,
    RENDER_CONTEXT_SWAPCHAIN_IMAGE_VIEWS_COMPONENT,
    RENDER_CONTEXT_DEPTH_IMAGE_VIEWS_COMPONENT,
    RENDER_CONTEXT_FRAMEBUFFERS_COMPONENT,
    RENDER_CONTEXT_IMAGE_AVAILABLE_COMPONENT,
    RENDER_CONTEXT_RENDER_FINISHED_COMPONENT,
    RENDER_CONTEXT_FRAME_IN_FLIGHT_COMPONENT,
    RENDER_CONTEXT_COMMAND_POOLS_COMPONENT,
    RENDER_CONTEXT_PRIMARY_COMMAND_BUFFERS_COMPONENT,
    RENDER_CONTEXT_COMPONENT_CAPACITY
} RendererContextComponent;

typedef struct RendererContext {
    uint32_t                    component_count;
    RendererContextComponent    components [RENDER_CONTEXT_COMPONENT_CAPACITY];
    VulkanDevice                device;
    VulkanRenderPass            render_pass;
    VulkanGraphicsPipeline      graphics_pipeline;
    VulkanSwapchain             swapchain;
    uint32_t                    image_count;
    uint32_t                    image_capacity;

    VkImage*                    swapchain_images;
    VkImage*                    depth_images;
    VkDeviceMemory*             depth_image_memories;
    VkImageView*                depth_image_views;
    VkImageView*                swapchain_image_views;
    VkFramebuffer*              framebuffers;
    uint32_t                    frame_count;
    VkSemaphore*                image_available;
    VkCommandPool*              command_pools;
    VkCommandBuffer*            primary_command_buffers;
    VkSemaphore*                render_finished;
    VkFence*                    in_flight;
    VulkanQueue                 main_queue;
    uint32_t                    image_index;
    uint32_t                    frame_index;
} RendererContext;

void RendererContext_Cleanup(RendererContext context [static 1]);

uint64_t RendererContex_CalculateRequiredBytes(const RendererContext context [static 1]);

typedef struct RendererContextCreateInfo {
    const bool                  debug;
    GLFWWindow*                 window;
} RendererContextCreateInfo;

RendererContext RendererContext_Create(const RendererContextCreateInfo create_info [static 1]);

bool RendererContext_Initialize(RendererContext contex [static 1], const uint32_t width, const uint32_t height, MemoryArena arena [static 1]);

typedef enum RendererComponent {
    RENDERER_WINDOW_COMPONENT,
    RENDERER_CONTEXT_COMPONENT,
    RENDERER_COMPONENT_CAPACITY
} RendererComponent;

typedef struct Renderer {
    uint32_t                    component_count;
    RendererComponent           components [RENDERER_COMPONENT_CAPACITY];
    GLFWWindow                  window;
    RendererContext             context;
} Renderer;

Renderer Renderer_Create();

uint64_t Renderer_CalculateRequiredBytes(const Renderer renderer [static 1]);

bool Renderer_Initialize(Renderer renderer [static 1], MemoryArena arena [static 1]);

void Renderer_Cleanup(Renderer renderer [static 1]);

bool Renderer_Loop(Renderer renderer [static 1]);

#endif