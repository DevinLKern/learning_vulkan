#ifndef ROSINA_ENGINE_RENDERER_H
#define ROSINA_ENGINE_RENDERER_H

#include <engine/backend/vulkan_helpers.h>
#include <utility/memory_arena.h>

#include <engine/graphics/window.h>

typedef enum RendererComponent
{
    RENDERER_LINK_COMPONENT,
    RENDERER_WINDOW_COMPONENT,
    RENDERER_CONTEXT_COMPONENT,
    RENDERER_DEVICE_COMPONENT,
    RENDERER_RENDER_PASS_COMPONENT,
    RENDERER_GRAPHICS_PIPELINE_COMPONENT,
    RENDERER_SWAPCHAIN_COMPONENT,
    RENDERER_MEMORY_COMPONENT,
    RENDERER_SWAPCHAIN_IMAGES_COMPONENT,
    RENDERER_DEPTH_IMAGES_COMPONENT,
    RENDERER_DEPTH_IMAGES_MEMORY_COMPONENT,
    RENDERER_SWAPCHAIN_IMAGE_VIEWS_COMPONENT,
    RENDERER_DEPTH_IMAGE_VIEWS_COMPONENT,
    RENDERER_FRAMEBUFFERS_COMPONENT,
    RENDERER_IMAGE_AVAILABLE_COMPONENT,
    RENDERER_RENDER_FINISHED_COMPONENT,
    RENDERER_FRAME_IN_FLIGHT_COMPONENT,
    RENDERER_COMMAND_POOLS_COMPONENT,
    RENDERER_PRIMARY_COMMAND_BUFFERS_COMPONENT,
    RENDERER_COMPONENT_CAPACITY
} RendererComponent;

typedef struct Renderer
{
    uint32_t component_count;
    RendererComponent components[RENDERER_COMPONENT_CAPACITY];
    GraphicsLink link;
    Window window;
    VulkanDevice device;
    VulkanRenderPass render_pass;
    VulkanGraphicsPipeline graphics_pipeline;
    VulkanSwapchain swapchain;

    uint32_t image_capacity;
    uint32_t image_count;
    uint32_t frame_capacity;
    uint32_t frame_count;
    MemoryArena memory;
    VkImage* swapchain_images;
    VkImage* depth_images;
    VkDeviceMemory depth_images_memory;
    VkImageView* depth_image_views;
    VkImageView* swapchain_image_views;
    VkFramebuffer* framebuffers;
    VkSemaphore* image_available;
    VkCommandPool* command_pools;
    VkCommandBuffer* primary_command_buffers;
    VkSemaphore* render_finished;
    VkFence* in_flight;

    uint32_t image_index;
    uint32_t frame_index;
} Renderer;

void Renderer_Cleanup(Renderer renderer[static 1]);

Renderer Renderer_Create();

bool Renderer_StartScene(Renderer renderer[static 1]);

bool Renderer_EndScene(Renderer renderer[static 1]);

#endif