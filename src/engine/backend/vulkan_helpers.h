#ifndef ROSINA_ENGINE_BACKEND_VULKAN_HELPERS_H
#define ROSINA_ENGINE_BACKEND_VULKAN_HELPERS_H

#include <engine/backend/glfw_helpers.h>

#include <utility/log.h>

#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

#include <stdbool.h>

#define VK_ERROR_RETURN(expression, return_value) {                     \
    const VkResult vulkan_return_result = expression;                   \
    if (vulkan_return_result != VK_SUCCESS) {                           \
        ROSINA_LOG_ERROR("%s", string_VkResult(vulkan_return_result));  \
        return return_value;                                            \
    }                                                                   \
}

#define VK_ERROR_HANDLE(expression, ___handles_VkError)                 \
{                                                                       \
    const VkResult vulkan_handle_result = expression;                   \
    if (vulkan_handle_result != VK_SUCCESS) {                           \
        ROSINA_LOG_ERROR("%s", string_VkResult(vulkan_handle_result));  \
        ___handles_VkError                                              \
    }                                                                   \
}

enum QueueCapabilityFlagBits {
    QUEUE_CAPABLITY_FLAG_GRAPHICS_BIT = (1 << 0),
    QUEUE_CAPABLITY_FLAG_TRANSFER_BIT = (1 << 1),
    QUEUE_CAPABLITY_FLAG_COMPUTE_BIT  = (1 << 2),
    QUEUE_CAPABLITY_FLAG_PRESENT_BIT  = (1 << 3)
};
typedef uint32_t QueueCapabilityFlags;

typedef enum VulkanDeviceComponent {
    VULKAN_DEVICE_COMPONENT_INSTANCE,
    VULKAN_DEVICE_COMPONENT_DEBUG_MESSENGER,
    VULKAN_DEVICE_COMPONENT_SURFACE,
    VULKAN_DEVICE_COMPONENT_DEVICE,
    VULKAN_DEVICE_COMPONENT_CAPACITY
} VulkanDeviceComponent;

typedef struct VulkanDevice {
    uint32_t                                component_count;
    VulkanDeviceComponent                   components [VULKAN_DEVICE_COMPONENT_CAPACITY];
    VkInstance                              instance;
    VkDebugUtilsMessengerEXT                debug_messenger;
    VkSurfaceKHR                            surface;
    VkPhysicalDevice                        physical_device;
    VkDevice                                handle;
} VulkanDevice;

void DestroyVulkanDevice(
    VulkanDevice                            device                  [static 1]
);

typedef struct VulkanDeviceCreateInfo {
    GLFWWindow*         window;
    uint32_t                                queue_description_count;
    QueueCapabilityFlags*                   queue_descriptions;
    bool                                    debug;
} VulkanDeviceCreateInfo;

bool CreateVulkanDevice(
    const VulkanDeviceCreateInfo            create_info             [static 1],
    VulkanDevice                            device                  [static 1]
);

typedef struct FindQueueFamilyIndexInfo {
    uint32_t                                queue_capability_count;
    QueueCapabilityFlags*                   queue_capabilities;
} FindQueueFamilyIndexInfo;

bool FindQueueFamilyIndices(
    const VulkanDevice                      device                  [static 1],
    const FindQueueFamilyIndexInfo          select_info             [static 1],
    uint32_t* const                         queue_family_indices
);

typedef struct VulkanQueue {
    VkQueue                                 handle;
    uint32_t                                family_index;
    uint32_t                                queue_index;
} VulkanQueue;

bool CreateVulkanImageMemories(
    const VulkanDevice                      device                  [static 1],
    const uint32_t                          image_count,
    const VkImage                           images                  [static image_count],
    VkDeviceMemory                          image_memories          [static image_count]
);

typedef struct VulkanImageViewsCreateInfo {
    uint32_t                                image_count;
    VkImage*                                images;
    VkImageViewCreateInfo*                  vk_image_create_info;
} VulkanImageViewsCreateInfo;

bool CreateVulkanImageViews(
    const VulkanDevice                      device                  [static 1],
    const VulkanImageViewsCreateInfo        create_info             [static 1],
    VkImageView*                            image_views
);

typedef struct VulkanRenderPass {
    VkSurfaceFormatKHR                      surface_format;
    VkFormat                                depth_format;
    VkRenderPass                            handle;
} VulkanRenderPass;

void DestroyVulkanRenderPass(
    const VulkanDevice                      device                  [static 1],
    VulkanRenderPass                        render_pass             [static 1]
);

bool CreateVulkanRenderPass(
    const VulkanDevice                      device                  [static 1],
    VulkanRenderPass                        render_pass             [static 1]
);

typedef enum VulkanGraphicsPipelineComponent {
    VULKAN_GRAPHICS_PIPELINE_VERTEX_SHADER_LAYOUT_COMPONENT,
    VULKAN_GRAPHICS_PIPELINE_FRAGMENT_SHADER_LAYOUT_COMPONENT,
    VULKAN_GRAPHICS_PIPELINE_LAYOUT_COMPONENT,
    VULKAN_GRAPHICS_PIPELINE_HANDLE_COMPONENT,
    VULKAN_GRAPHICS_PIPELINE_COMPONENT_CAPACITY
} VulkanGraphicsPipelineComponent;

typedef struct VulkanGraphicsPipeline {
    uint32_t                                component_count;
    VulkanGraphicsPipelineComponent         components [VULKAN_GRAPHICS_PIPELINE_COMPONENT_CAPACITY];
    VkDescriptorSetLayout                   vertex_shader_layout;
    VkDescriptorSetLayout                   fragment_shader_layout; 
    VkPipelineLayout                        layout;
    VkPipeline                              handle;
} VulkanGraphicsPipeline;

void DestroyVulkanGraphicsPipeline(
    const VulkanDevice                      device                  [static 1],
    VulkanGraphicsPipeline                  pipeline                [static 1]
);

typedef struct InputDescription {
    VkFormat                                format;
    uint32_t                                offset;
} InputDescription;

typedef struct VulkanGraphicsPipelineCreateInfo {
    char*                                   vertex_shader_path;
    char*                                   fragment_shader_path;
    VulkanRenderPass*                       render_pass;
    uint32_t                                vertex_input_description_count;
    InputDescription*                       vertex_input_descriptions;
    uint32_t                                width;
    uint32_t                                height;
} VulkanGraphicsPipelineCreateInfo;

bool CreateVulkanGraphicsPipeline(
    const VulkanDevice                      device                  [static 1],
    const VulkanGraphicsPipelineCreateInfo  create_info             [static 1],
    VulkanGraphicsPipeline                  pipeline                [static 1]
);

typedef struct VulkanSwapchain {
    VkPresentModeKHR                        present_mode;
    VkExtent2D                              extent;
    VkSwapchainKHR                          handle;
} VulkanSwapchain;

void DestroyVulkanSwapchain(
    const VulkanDevice                      device                  [static 1],
    VulkanSwapchain                         swapchain               [static 1]
);

typedef struct VulkanSwapchainCreateInfo {
    uint32_t*                               queue_family_indices;
    uint32_t                                queue_family_index_count;
    uint32_t                                width;
    uint32_t                                height;
    VulkanRenderPass*                       render_pass;
} VulkanSwapchainCreateInfo;

bool CreateVulkanSwapchain(
    const VulkanDevice                      device                  [static 1], 
    const VulkanSwapchainCreateInfo         create_info             [static 1], 
    VulkanSwapchain                         swapchain               [static 1]
);

typedef struct VulkanBuffer {
    VkBuffer                                handle;
    VkDeviceMemory                          memory;
} VulkanBuffer;

void DestroyVulkanBuffer(
    const VulkanDevice                      device                  [static 1], 
    VulkanBuffer                            buffer                  [static 1]
);

typedef struct VulkanBufferCreateInfo {
    VkDeviceSize                            size;
    VkBufferUsageFlags                      usage;
    VkSharingMode                           mode;
    VkMemoryPropertyFlags                   properties;
} VulkanBufferCreateInfo;

bool CreateVulkanBuffer(
    const VulkanDevice                      device                  [static 1],
    const VulkanBufferCreateInfo            create_info             [static 1],
    VulkanBuffer                            buffer                  [static 1]
);

bool CreateVulkanStagingBuffer(
    const VulkanDevice                      device                  [static 1],
    const VkDeviceSize                      size,
    VulkanBuffer                            buffer                  [static 1]
);

bool CreateVulkanVertexBuffer(
    const VulkanDevice                      device                  [static 1],
    const VkDeviceSize                      size,
    VulkanBuffer                            buffer                  [static 1]
);

bool CreateVulkanIndexBuffer(
    const VulkanDevice                      device                  [static 1],
    const VkDeviceSize                      size,
    VulkanBuffer                            buffer                  [static 1]
);

bool CreateVulkanUniformBuffer(
    const VulkanDevice                      device                  [static 1],
    const VkDeviceSize                      size,
    VulkanBuffer                            buffer                  [static 1]
);

#endif