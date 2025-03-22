#include <assert.h>
#include <engine/backend/vulkan_helpers.h>
#include <string.h>
#include <utility/load_file.h>

void DestroyVulkanGraphicsPipeline(const VulkanDevice device[static 1], VulkanGraphicsPipeline pipeline[static 1])
{
    vkDestroyPipeline(device->handle, pipeline->handle, NULL);
    pipeline->handle = VK_NULL_HANDLE;
    vkDestroyPipelineLayout(device->handle, pipeline->layout, NULL);
    pipeline->layout = VK_NULL_HANDLE;
}

