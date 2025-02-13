#include <assert.h>
#include <engine/backend/vulkan_helpers.h>
#include <stdlib.h>

void VulkanRenderPass_Cleanup(const VulkanDevice device[static 1], VulkanRenderPass render_pass[static 1])
{
    vkDestroyRenderPass(device->handle, render_pass->handle, NULL);
}

static inline bool SelectVkSurfaceFormat(const VkSurfaceKHR surface, const VkPhysicalDevice physical_device, VkSurfaceFormatKHR surface_format[static 1])
{
    uint32_t supported_surface_format_count = 0;
    VK_ERROR_RETURN(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &supported_surface_format_count, NULL), 0);
    VkSurfaceFormatKHR* const supported_surface_formats = calloc(supported_surface_format_count, sizeof(VkSurfaceFormatKHR));
    VK_ERROR_RETURN(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &supported_surface_format_count, supported_surface_formats), 0);

    if (supported_surface_format_count == 0)
    {
        free(supported_surface_formats);
        return true;
    }
    else if (supported_surface_format_count == 1 && supported_surface_formats[0].format == VK_FORMAT_UNDEFINED)
    {
        surface_format->format     = VK_FORMAT_B8G8R8A8_UNORM;
        surface_format->colorSpace = supported_surface_formats[0].colorSpace;
    }
    else
    {  // TODO: select ideal surface format?
        *surface_format = supported_surface_formats[0];
    }

    free(supported_surface_formats);
    return false;
}

static inline bool SelectDepthFormat(const VkPhysicalDevice physical_device, VkFormat* const depth_format)
{
    VkFormatProperties properties = {};
    vkGetPhysicalDeviceFormatProperties(physical_device, VK_FORMAT_D32_SFLOAT_S8_UINT, &properties);
    const bool d32s8_support = (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    if (d32s8_support)
    {
        *depth_format = VK_FORMAT_D32_SFLOAT_S8_UINT;
        return false;
    }

    vkGetPhysicalDeviceFormatProperties(physical_device, VK_FORMAT_D24_UNORM_S8_UINT, &properties);
    const bool d24s8_support = (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    if (d24s8_support)
    {
        *depth_format = VK_FORMAT_D24_UNORM_S8_UINT;
        return false;
    }

    return true;
}

static inline bool CreateVkRenderPass(const VkSurfaceFormatKHR surface_format, const VkFormat depth_format, const VulkanDevice device[static 1],
                                      VkRenderPass render_pass[static 1])
{
    const VkAttachmentDescription2 attachments[] = {{.sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
                                                     .pNext          = NULL,
                                                     .flags          = 0,
                                                     .format         = surface_format.format,
                                                     .samples        = VK_SAMPLE_COUNT_1_BIT,
                                                     .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                     .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                                                     .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                                     .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
                                                     .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,  // could be VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ?
                                                     .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR},
                                                    {.sType          = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
                                                     .pNext          = NULL,
                                                     .flags          = 0,
                                                     .format         = depth_format,
                                                     .samples        = VK_SAMPLE_COUNT_1_BIT,
                                                     .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                     .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                                                     .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                                     .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                                     .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
                                                     .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR}};

    const VkAttachmentReference2 color_attachment_refs[] = {{
        .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
        .pNext      = NULL,
        .attachment = 0,
        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .aspectMask = surface_format.format,
    }};
    const VkAttachmentReference2 depth_attachment_refs[] = {{
        .sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
        .pNext      = NULL,
        .attachment = 1,
        .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .aspectMask = depth_format,
    }};
    const VkSubpassDescription2 subpasses[]              = {{
                     .sType                   = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
                     .pNext                   = NULL,
                     .flags                   = 0,
                     .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
                     .viewMask                = 0,
                     .inputAttachmentCount    = 0,
                     .pInputAttachments       = NULL,
                     .colorAttachmentCount    = sizeof(color_attachment_refs) / sizeof(VkAttachmentReference2),
                     .pColorAttachments       = color_attachment_refs,
                     .pResolveAttachments     = NULL,
                     .pDepthStencilAttachment = depth_attachment_refs,
                     .preserveAttachmentCount = 0,
                     .pPreserveAttachments    = NULL,
    }};

    const VkSubpassDependency2 dependencies[] = {{.sType           = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
                                                  .pNext           = NULL,
                                                  .srcSubpass      = 0,
                                                  .dstSubpass      = VK_SUBPASS_EXTERNAL,
                                                  .srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                                  .dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                                  .srcAccessMask   = 0,
                                                  .dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                                  .dependencyFlags = 0,
                                                  .viewOffset      = 0}};

    const VkRenderPassCreateInfo2 create_info = {
        .sType                   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
        .pNext                   = NULL,
        .flags                   = 0,
        .attachmentCount         = sizeof(attachments) / sizeof(VkAttachmentDescription2),
        .pAttachments            = attachments,
        .subpassCount            = sizeof(subpasses) / sizeof(VkSubpassDescription2),
        .pSubpasses              = subpasses,
        .dependencyCount         = sizeof(dependencies) / sizeof(VkSubpassDependency2),
        .pDependencies           = dependencies,
        .correlatedViewMaskCount = 0,
        .pCorrelatedViewMasks    = 0,
    };

    VK_ERROR_RETURN(vkCreateRenderPass2(device->handle, &create_info, NULL, render_pass), true);
    return false;
}

bool VulkanRenderPass_Create(const VulkanDevice device[static 1], const VkSurfaceKHR surface, VulkanRenderPass render_pass[static 1])
{
    if (SelectVkSurfaceFormat(surface, device->physical_device, &render_pass->surface_format))
    {
        ROSINA_LOG_ERROR("Could not select VkSurfaceFormatKHR");
        return true;
    }

    if (SelectDepthFormat(device->physical_device, &render_pass->depth_format))
    {
        ROSINA_LOG_ERROR("Could not select depth format");
        return true;
    }

    if (CreateVkRenderPass(render_pass->surface_format, render_pass->depth_format, device, &render_pass->handle))
    {
        ROSINA_LOG_ERROR("Could not create render pass");
        return true;
    }

    return false;
}