#ifndef ROSINA_ENGINE_VULKAN_HELPERS_H
#define ROSINA_ENGINE_VULKAN_HELPERS_H

#include <stdbool.h>
#include <utility/log.h>
#include <vulkan/vulkan.h>

// this is from vk_enum_string_helper.h, which only is written in C++.
const char* string_VkResult(VkResult input_value);

#define VK_ERROR_RETURN(expression, return_value)                          \
    {                                                                      \
        const VkResult vulkan_return_result = expression;                  \
        if (vulkan_return_result != VK_SUCCESS)                            \
        {                                                                  \
            ROSINA_LOG_ERROR("%s", string_VkResult(vulkan_return_result)); \
            return return_value;                                           \
        }                                                                  \
    }

#define VK_ERROR_HANDLE(expression, ___handles_VkError)                    \
    {                                                                      \
        const VkResult vulkan_handle_result = expression;                  \
        if (vulkan_handle_result != VK_SUCCESS)                            \
        {                                                                  \
            ROSINA_LOG_ERROR("%s", string_VkResult(vulkan_handle_result)); \
            ___handles_VkError                                             \
        }                                                                  \
    }

uint32_t FindMemoryType(const VkPhysicalDevice physical_device, const uint32_t type_filter, const VkMemoryPropertyFlags properties);

enum QueueCapabilityFlagBits
{
    QUEUE_CAPABILITY_FLAG_GRAPHICS_BIT = (1 << 0),
    QUEUE_CAPABILITY_FLAG_TRANSFER_BIT = (1 << 1),
    QUEUE_CAPABILITY_FLAG_COMPUTE_BIT  = (1 << 2),
    QUEUE_CAPABILITY_FLAG_PRESENT_BIT  = (1 << 3)
};
typedef uint32_t QueueCapabilityFlags;

typedef struct VulkanQueue
{
    VkQueue handle;
    uint32_t family_index;
    uint32_t queue_index;
} VulkanQueue;

typedef struct VulkanDevice
{
    VkPhysicalDevice physical_device;
    VkDevice handle;
    VulkanQueue graphics_queue;
    VulkanQueue transfer_queue;
    VulkanQueue present_queue;
} VulkanDevice;

void VulkanDevice_Cleanup(VulkanDevice device[static 1]);

typedef struct FindQueueFamilyIndexInfo
{
    QueueCapabilityFlags flags;
    uint32_t queue_count;
    VkSurfaceKHR surface;
} FindQueueFamilyIndexInfo;

/**
 * Returns queue family index or UINT32_MAX on error
 */
uint32_t FindQueueFamilyIndex(const VulkanDevice device[static 1], const FindQueueFamilyIndexInfo info[static 1]);

typedef struct VulkanDeviceCreateInfo
{
    QueueCapabilityFlags queue_capabilities;
    VkInstance instance;
    VkSurfaceKHR surface;
} VulkanDeviceCreateInfo;

bool CreateVulkanDevice(const VulkanDeviceCreateInfo create_info[static 1], VulkanDevice device[static 1]);

typedef struct VulkanRenderPass
{
    VkSurfaceFormatKHR surface_format;
    VkFormat depth_format;
    VkRenderPass handle;
} VulkanRenderPass;

void VulkanRenderPass_Cleanup(const VulkanDevice device[static 1], VulkanRenderPass render_pass[static 1]);

bool VulkanRenderPass_Create(const VulkanDevice device[static 1], const VkSurfaceKHR surface, VulkanRenderPass render_pass[static 1]);

typedef enum VulkanGraphicsPipelineComponent
{
    VULKAN_GRAPHICS_PIPELINE_LAYOUT_COMPONENT,
    VULKAN_GRAPHICS_PIPELINE_HANDLE_COMPONENT,
    VULKAN_GRAPHICS_PIPELINE_COMPONENT_CAPACITY
} VulkanGraphicsPipelineComponent;

typedef struct VulkanGraphicsPipeline
{
    VkPipelineLayout layout;
    VkPipeline handle;
} VulkanGraphicsPipeline;

void DestroyVulkanGraphicsPipeline(const VulkanDevice device[static 1], VulkanGraphicsPipeline pipeline[static 1]);

typedef struct InputDescription
{
    VkFormat format;
    uint32_t offset;
} InputDescription;

typedef struct VulkanGraphicsPipelineCreateInfo
{
    VulkanRenderPass* render_pass;
    VkShaderModule vertex_shader_module;
    VkShaderModule fragment_shader_module;
    VkDescriptorSetLayout shader_layout;
    uint32_t width;
    uint32_t height;
} VulkanGraphicsPipelineCreateInfo;


typedef struct VulkanSwapchain
{
    VkPresentModeKHR present_mode;
    VkExtent2D extent;
    VkSwapchainKHR handle;
} VulkanSwapchain;

void VulkanSwapchain_Cleanup(const VulkanDevice device[static 1], VulkanSwapchain swapchain[static 1]);

typedef struct VulkanSwapchainCreateInfo
{
    uint32_t* queue_family_indices;
    uint32_t queue_family_index_count;
    uint32_t width;
    uint32_t height;
    VkSurfaceKHR surface;
    VulkanRenderPass* render_pass;
} VulkanSwapchainCreateInfo;

bool VulkanSwapchain_Create(const VulkanDevice device[static 1], const VulkanSwapchainCreateInfo create_info[static 1], VulkanSwapchain swapchain[static 1]);

#endif