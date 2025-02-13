#include <assert.h>
#include <engine/graphics/graphics.h>
#include <stdlib.h>

void Renderer_Cleanup(Renderer renderer[static 1])
{
    while (renderer->component_count > 0)
    {
        switch (renderer->components[--renderer->component_count])
        {
            case RENDERER_LINK_COMPONENT:
                GraphicsLink_Cleanup(&renderer->link);
                break;
            case RENDERER_WINDOW_COMPONENT:
                Window_Cleanup(&renderer->link, &renderer->window);
                break;
            case RENDERER_DEVICE_COMPONENT:
                VulkanDevice_Cleanup(&renderer->device);
                break;
            case RENDERER_RENDER_PASS_COMPONENT:
                VulkanRenderPass_Cleanup(&renderer->device, &renderer->render_pass);
                break;
            case RENDERER_SWAPCHAIN_COMPONENT:
                VulkanSwapchain_Cleanup(&renderer->device, &renderer->swapchain);
                break;
            case RENDERER_MEMORY_COMPONENT:
                MemoryArena_Free(&renderer->memory);
                break;
            case RENDERER_SWAPCHAIN_IMAGES_COMPONENT:
                for (uint32_t i = 0; i < renderer->image_count; i++)
                {
                    renderer->swapchain_images[i] = VK_NULL_HANDLE;
                }
                break;
            case RENDERER_DEPTH_IMAGES_COMPONENT:
                for (uint32_t i = 0; i < renderer->image_count; i++)
                {
                    vkDestroyImage(renderer->device.handle, renderer->depth_images[i], NULL);
                    renderer->depth_images[i] = VK_NULL_HANDLE;
                }
                break;
            case RENDERER_DEPTH_IMAGES_MEMORY_COMPONENT:
                vkFreeMemory(renderer->device.handle, renderer->depth_images_memory, NULL);
                renderer->depth_images_memory = VK_NULL_HANDLE;
                break;
            case RENDERER_DEPTH_IMAGE_VIEWS_COMPONENT:
                for (uint32_t i = 0; i < renderer->image_count; i++)
                {
                    vkDestroyImageView(renderer->device.handle, renderer->depth_image_views[i], NULL);
                    renderer->depth_image_views[i] = VK_NULL_HANDLE;
                }
                break;
            case RENDERER_SWAPCHAIN_IMAGE_VIEWS_COMPONENT:
                for (uint32_t i = 0; i < renderer->image_count; i++)
                {
                    vkDestroyImageView(renderer->device.handle, renderer->swapchain_image_views[i], NULL);
                    renderer->swapchain_image_views[i] = VK_NULL_HANDLE;
                }
                break;
            case RENDERER_FRAMEBUFFERS_COMPONENT:
                for (uint32_t i = 0; i < renderer->image_count; i++)
                {
                    vkDestroyFramebuffer(renderer->device.handle, renderer->framebuffers[i], NULL);
                    renderer->framebuffers[i] = VK_NULL_HANDLE;
                }
                break;
            case RENDERER_FRAME_IN_FLIGHT_COMPONENT:
                for (uint32_t i = 0; i < renderer->image_count; i++)
                {
                    vkDestroyFence(renderer->device.handle, renderer->in_flight[i], NULL);
                    renderer->in_flight[i] = VK_NULL_HANDLE;
                }
                break;
            case RENDERER_IMAGE_AVAILABLE_COMPONENT:
                for (uint32_t i = 0; i < renderer->frame_count; i++)
                {
                    vkDestroySemaphore(renderer->device.handle, renderer->image_available[i], NULL);
                    renderer->image_available[i] = VK_NULL_HANDLE;
                }
                break;
            case RENDERER_RENDER_FINISHED_COMPONENT:
                for (uint32_t i = 0; i < renderer->frame_count; i++)
                {
                    vkDestroySemaphore(renderer->device.handle, renderer->render_finished[i], NULL);
                    renderer->render_finished[i] = VK_NULL_HANDLE;
                }
                break;
            case RENDERER_GRAPHICS_PIPELINE_COMPONENT:
                DestroyVulkanGraphicsPipeline(&renderer->device, &renderer->graphics_pipeline);
                break;
            case RENDERER_COMMAND_POOLS_COMPONENT:
                for (uint32_t i = 0; i < renderer->frame_count; i++)
                {
                    vkDestroyCommandPool(renderer->device.handle, renderer->command_pools[i], NULL);
                    renderer->command_pools[i] = VK_NULL_HANDLE;
                }
                break;
            default:
                ROSINA_LOG_ERROR("Invalid renderer component value");
                assert(false);
        }
    }
}

Renderer Renderer_Create()
{
    Renderer renderer = {
        .component_count = 0,
        .components      = {},
    };

    {
        renderer.link = GraphicsLink_Create(true);
        if (renderer.link.instance == VK_NULL_HANDLE)
        {
            ROSINA_LOG_ERROR("Could not create graphics link!");
            Renderer_Cleanup(&renderer);
            return renderer;
        }
        renderer.components[renderer.component_count++] = RENDERER_LINK_COMPONENT;
    }

    {
        const WindowCreateInfo window_create_info = {.width = 1000, .height = 1000, .title = "GLFWWindow", .link = &renderer.link};
        renderer.window                           = Window_Create(&window_create_info);
        if (renderer.window.handle == NULL)
        {
            ROSINA_LOG_ERROR("Could not create renderer window!");
            return renderer;
        }

        renderer.components[renderer.component_count++] = RENDERER_WINDOW_COMPONENT;
    }

    {
        // create vulkan device
        {
            const VulkanDeviceCreateInfo device_create_info = {.queue_capabilities = QUEUE_CAPABLITY_FLAG_GRAPHICS_BIT | QUEUE_CAPABLITY_FLAG_PRESENT_BIT,
                                                               .instance           = renderer.link.instance,
                                                               .surface            = renderer.window.surface};
            if (CreateVulkanDevice(&device_create_info, &renderer.device))
            {
                ROSINA_LOG_ERROR("Failed to create VulkanDevice");
                Renderer_Cleanup(&renderer);
                return renderer;
            }

            renderer.components[renderer.component_count++] = RENDERER_DEVICE_COMPONENT;
        }
    }

    // Create render_pass
    {
        if (VulkanRenderPass_Create(&renderer.device, renderer.window.surface, &renderer.render_pass))
        {
            ROSINA_LOG_ERROR("Could not create render pass");
            Renderer_Cleanup(&renderer);
            return renderer;
        }
        renderer.components[renderer.component_count++] = RENDERER_RENDER_PASS_COMPONENT;
    }

    // Create swapchain
    {
        const VulkanSwapchainCreateInfo swapchain_create_info = {
            .width = renderer.window.width, .height = renderer.window.height, .render_pass = &renderer.render_pass, .surface = renderer.window.surface};
        renderer.swapchain.present_mode = VK_PRESENT_MODE_FIFO_KHR;
        if (VulkanSwapchain_Create(&renderer.device, &swapchain_create_info, &renderer.swapchain))
        {
            ROSINA_LOG_ERROR("Failed to create VulkanSwapchain");
            Renderer_Cleanup(&renderer);
            return renderer;
        }
        renderer.components[renderer.component_count++] = RENDERER_SWAPCHAIN_COMPONENT;
    }

    // Calculate image_count, image_capacity, frame_count and frame_capacity
    {
        renderer.frame_capacity = 3;
        renderer.frame_count    = renderer.swapchain.present_mode == VK_PRESENT_MODE_FIFO_KHR ? 2 : 3;

        VK_ERROR_HANDLE(vkGetSwapchainImagesKHR(renderer.device.handle, renderer.swapchain.handle, &renderer.image_count, NULL), {
            Renderer_Cleanup(&renderer);
            return renderer;
        });
        {
            VkSurfaceCapabilitiesKHR capabilities = {};
            VK_ERROR_HANDLE(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderer.device.physical_device, renderer.window.surface, &capabilities), {
                Renderer_Cleanup(&renderer);
                return renderer;
            });
            renderer.image_capacity = capabilities.maxImageCount > 0 ? capabilities.maxImageCount : renderer.image_count + 3;
        }
    }

    // create arena required bytes
    {
        uint64_t required_bytes = (renderer.frame_capacity * (sizeof(VkSemaphore) +      // renderer.image_available
                                                              sizeof(VkCommandPool) +    // renderer.command_pools
                                                              sizeof(VkCommandBuffer) +  // renderer.main_buffers
                                                              sizeof(VkSemaphore) +      // renderer.render_finished
                                                              sizeof(VkFence)            // renderer.in_flight
                                                              )) +
                                  (renderer.image_capacity * (sizeof(VkImage) +      // renderer.swapchain_images
                                                              sizeof(VkImage) +      // renderer.depth_images
                                                              sizeof(VkImageView) +  // renderer.swapchain_image_views
                                                              sizeof(VkImageView) +  // renderer.depth_image_views
                                                              sizeof(VkFramebuffer)  // renderer.framebuffers
                                                              ));
        renderer.memory                                 = MemoryArena_Create(required_bytes);
        renderer.components[renderer.component_count++] = RENDERER_MEMORY_COMPONENT;
    }

    // get swapchain images
    {
        renderer.swapchain_images = MemoryArena_Allocate(&renderer.memory, renderer.image_capacity * sizeof(VkImage));

        VK_ERROR_HANDLE(vkGetSwapchainImagesKHR(renderer.device.handle, renderer.swapchain.handle, &renderer.image_count, NULL), {
            Renderer_Cleanup(&renderer);
            return renderer;
        });

        if (renderer.image_count > renderer.image_capacity)
        {
            ROSINA_LOG_ERROR("I hate my life");
            Renderer_Cleanup(&renderer);
            return renderer;
        }

        VK_ERROR_HANDLE(vkGetSwapchainImagesKHR(renderer.device.handle, renderer.swapchain.handle, &renderer.image_count, renderer.swapchain_images), {
            Renderer_Cleanup(&renderer);
            return renderer;
        });

        renderer.components[renderer.component_count++] = RENDERER_SWAPCHAIN_IMAGES_COMPONENT;
    }

    // create depth images
    {
        renderer.depth_images = MemoryArena_Allocate(&renderer.memory, renderer.image_capacity * sizeof(VkImage));

        const VkImageCreateInfo image_create_info = {
            .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext                 = NULL,
            .flags                 = 0,
            .imageType             = VK_IMAGE_TYPE_2D,
            .format                = renderer.render_pass.depth_format,
            .extent                = {.width = renderer.swapchain.extent.width, .height = renderer.swapchain.extent.height, .depth = 1},
            .mipLevels             = 1,
            .arrayLayers           = 1,
            .samples               = VK_SAMPLE_COUNT_1_BIT,
            .tiling                = VK_IMAGE_TILING_OPTIMAL,  // Could be VK_IMAGE_TILING_LINEAR ?
            .usage                 = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,     // Read the docs
            .pQueueFamilyIndices   = NULL,  // Read the docs
            .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED};

        for (uint32_t i = 0; i < renderer.image_count; i++)
        {
            VK_ERROR_HANDLE(vkCreateImage(renderer.device.handle, &image_create_info, NULL, renderer.depth_images + i), {
                for (uint32_t j = 0; j < i; j++)
                {
                    vkDestroyImage(renderer.device.handle, renderer.depth_images[j], NULL);
                }
                Renderer_Cleanup(&renderer);
                return renderer;
            });
        }

        renderer.components[renderer.component_count++] = RENDERER_DEPTH_IMAGES_COMPONENT;
    }

    // allocate memory for depth images
    {
        renderer.depth_images_memory = VK_NULL_HANDLE;

        VkDeviceSize depth_images_memory_size = 0;
        uint32_t depth_image_memory_type_bits = 0;
        for (uint32_t i = 0; i < renderer.image_count; i++)
        {
            VkMemoryRequirements memory_requirements;
            vkGetImageMemoryRequirements(renderer.device.handle, renderer.depth_images[i], &memory_requirements);
            depth_images_memory_size += memory_requirements.size;
            depth_image_memory_type_bits |= memory_requirements.memoryTypeBits;
        }

        const VkMemoryAllocateInfo alloc_info = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize  = depth_images_memory_size,
            .memoryTypeIndex = FindMemoryType(renderer.device.physical_device, depth_image_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

        VK_ERROR_HANDLE(vkAllocateMemory(renderer.device.handle, &alloc_info, NULL, &renderer.depth_images_memory), {
            Renderer_Cleanup(&renderer);
            return renderer;
        });

        depth_images_memory_size = 0;
        for (uint32_t i = 0; i < renderer.image_count; i++)
        {
            VkMemoryRequirements memory_requirements;
            vkGetImageMemoryRequirements(renderer.device.handle, renderer.depth_images[i], &memory_requirements);

            VK_ERROR_HANDLE(vkBindImageMemory(renderer.device.handle, renderer.depth_images[i], renderer.depth_images_memory, depth_images_memory_size), {
                vkFreeMemory(renderer.device.handle, renderer.depth_images_memory, NULL);
                renderer.depth_images_memory = VK_NULL_HANDLE;
                Renderer_Cleanup(&renderer);
                return renderer;
            });
        }

        renderer.components[renderer.component_count++] = RENDERER_DEPTH_IMAGES_MEMORY_COMPONENT;
    }

    // create depth image views
    {
        renderer.depth_image_views = MemoryArena_Allocate(&renderer.memory, renderer.image_capacity * sizeof(VkImageView));

        for (uint32_t i = 0; i < renderer.image_count; i++)
        {
            const VkImageViewCreateInfo image_view_create_info = {
                .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext            = NULL,
                .flags            = 0,
                .image            = renderer.depth_images[i],
                .viewType         = VK_IMAGE_VIEW_TYPE_2D,
                .format           = renderer.render_pass.depth_format,
                .components       = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                                     .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                                     .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                     .a = VK_COMPONENT_SWIZZLE_IDENTITY},
                .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};
            VK_ERROR_HANDLE(vkCreateImageView(renderer.device.handle, &image_view_create_info, NULL, renderer.depth_image_views + i), {
                for (uint32_t j = 0; j < i; j++)
                {
                    vkDestroyImageView(renderer.device.handle, renderer.depth_image_views[i], NULL);
                }
                Renderer_Cleanup(&renderer);
                return renderer;
            });
        }

        renderer.components[renderer.component_count++] = RENDERER_DEPTH_IMAGE_VIEWS_COMPONENT;
    }

    // create swapchain image views
    {
        renderer.swapchain_image_views = MemoryArena_Allocate(&renderer.memory, renderer.image_capacity * sizeof(VkImageView));

        VkImageViewCreateInfo image_view_create_info = {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext            = NULL,
            .flags            = 0,
            .image            = VK_NULL_HANDLE,
            .viewType         = VK_IMAGE_VIEW_TYPE_2D,
            .format           = renderer.render_pass.surface_format.format,
            .components       = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                                 .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                                 .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                 .a = VK_COMPONENT_SWIZZLE_IDENTITY},
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};

        for (uint32_t i = 0; i < renderer.image_count; i++)
        {
            image_view_create_info.image = renderer.swapchain_images[i];
            VK_ERROR_HANDLE(vkCreateImageView(renderer.device.handle, &image_view_create_info, NULL, renderer.swapchain_image_views + i), {
                for (uint32_t j = 0; j < i; j++)
                {
                    vkDestroyImageView(renderer.device.handle, renderer.swapchain_image_views[i], NULL);
                }
                Renderer_Cleanup(&renderer);
                return renderer;
            });
        }

        renderer.components[renderer.component_count++] = RENDERER_SWAPCHAIN_IMAGE_VIEWS_COMPONENT;
    }

    // create framebuffers
    {
        renderer.framebuffers = MemoryArena_Allocate(&renderer.memory, renderer.image_capacity * sizeof(VkFramebuffer));

        for (uint32_t i = 0; i < renderer.image_count; i++)
        {
            const VkImageView attachments[]                       = {renderer.swapchain_image_views[i], renderer.depth_image_views[i]};
            const VkFramebufferCreateInfo framebuffer_create_info = {.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                                                                     .pNext           = NULL,
                                                                     .flags           = 0,
                                                                     .renderPass      = renderer.render_pass.handle,
                                                                     .attachmentCount = sizeof(attachments) / sizeof(VkImageView),
                                                                     .pAttachments    = attachments,
                                                                     .width           = renderer.window.width,
                                                                     .height          = renderer.window.height,
                                                                     .layers          = 1};
            VK_ERROR_HANDLE(vkCreateFramebuffer(renderer.device.handle, &framebuffer_create_info, NULL, renderer.framebuffers + i), {
                for (uint32_t j = 0; j < i; j++)
                {
                    vkDestroyFramebuffer(renderer.device.handle, renderer.framebuffers[j], NULL);
                }
                Renderer_Cleanup(&renderer);
                return renderer;
            });
        }

        renderer.components[renderer.component_count++] = RENDERER_FRAMEBUFFERS_COMPONENT;
    }

    // create in flight fences
    {
        renderer.in_flight = MemoryArena_Allocate(&renderer.memory, renderer.frame_capacity * sizeof(VkFence));

        const VkFenceCreateInfo fence_create_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = NULL,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT  // VK_FENCE_CREATE_SIGNALED_BIT
        };
        for (uint32_t i = 0; i < renderer.frame_count; i++)
        {
            VK_ERROR_HANDLE(vkCreateFence(renderer.device.handle, &fence_create_info, NULL, renderer.in_flight + i), {
                for (uint32_t j = 0; j < i; j++)
                {
                    vkDestroyFence(renderer.device.handle, renderer.in_flight[j], NULL);
                }
                Renderer_Cleanup(&renderer);
                return renderer;
            });
        }

        renderer.components[renderer.component_count++] = RENDERER_FRAME_IN_FLIGHT_COMPONENT;
    }

    // create image available semaphores
    {
        renderer.image_available = MemoryArena_Allocate(&renderer.memory, renderer.frame_capacity * sizeof(VkSemaphore));

        const VkSemaphoreCreateInfo semaphore_create_info = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = NULL, .flags = 0};
        for (uint32_t i = 0; i < renderer.frame_count; i++)
        {
            VK_ERROR_HANDLE(vkCreateSemaphore(renderer.device.handle, &semaphore_create_info, NULL, renderer.image_available + i), {
                for (uint32_t j = 0; j < i; j++)
                {
                    vkDestroySemaphore(renderer.device.handle, renderer.image_available[j], NULL);
                }
                Renderer_Cleanup(&renderer);
                return renderer;
            });
        }

        renderer.components[renderer.component_count++] = RENDERER_IMAGE_AVAILABLE_COMPONENT;
    }

    // create render finished semaphores
    {
        renderer.render_finished = MemoryArena_Allocate(&renderer.memory, renderer.frame_capacity * sizeof(VkSemaphore));

        const VkSemaphoreCreateInfo semaphore_create_info = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = NULL, .flags = 0};
        for (uint32_t i = 0; i < renderer.frame_count; i++)
        {
            VK_ERROR_HANDLE(vkCreateSemaphore(renderer.device.handle, &semaphore_create_info, NULL, renderer.render_finished + i), {
                for (uint32_t j = 0; j < i; j++)
                {
                    vkDestroySemaphore(renderer.device.handle, renderer.render_finished[j], NULL);
                }
                Renderer_Cleanup(&renderer);
                return renderer;
            });
        }

        renderer.components[renderer.component_count++] = RENDERER_RENDER_FINISHED_COMPONENT;
    }

    // create command pools
    {
        renderer.command_pools = MemoryArena_Allocate(&renderer.memory, renderer.frame_capacity * sizeof(VkCommandPool));

        const VkCommandPoolCreateInfo pool_create_info = {.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                                          .pNext            = NULL,
                                                          .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                                                          .queueFamilyIndex = renderer.device.graphics_queue.family_index};
        for (uint32_t i = 0; i < renderer.frame_count; i++)
        {
            VK_ERROR_HANDLE(vkCreateCommandPool(renderer.device.handle, &pool_create_info, NULL, renderer.command_pools + i), {
                for (uint32_t j = 0; j < i; j++)
                {
                    vkDestroyCommandPool(renderer.device.handle, renderer.command_pools[j], NULL);
                }
                Renderer_Cleanup(&renderer);
                return renderer;
            });
        }

        renderer.components[renderer.component_count++] = RENDERER_COMMAND_POOLS_COMPONENT;
    }

    // primary command buffers
    {
        renderer.primary_command_buffers = MemoryArena_Allocate(&renderer.memory, renderer.frame_capacity * sizeof(VkCommandBuffer));

        for (uint32_t i = 0; i < renderer.frame_count; i++)
        {
            const VkCommandBufferAllocateInfo alloc_info = {.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                                            .pNext              = NULL,
                                                            .commandPool        = renderer.command_pools[i],
                                                            .level              = 0,
                                                            .commandBufferCount = 1};
            VK_ERROR_HANDLE(vkAllocateCommandBuffers(renderer.device.handle, &alloc_info, renderer.primary_command_buffers + i), {
                for (uint32_t j = 0; j < i; j++)
                {
                    vkFreeCommandBuffers(renderer.device.handle, renderer.command_pools[j], 1, renderer.primary_command_buffers + j);
                }
                Renderer_Cleanup(&renderer);
                return renderer;
            });
        }
    }

    return renderer;
}

bool Renderer_InitializeGraphicsPipeline(Renderer renderer[static 1], Shader shader[static 1])
{
    const VulkanGraphicsPipelineCreateInfo pipeline_create_info = {.render_pass            = &renderer->render_pass,
                                                                   .vertex_shader_module   = shader->vertex_module,
                                                                   .fragment_shader_module = shader->fragment_module,
                                                                   .vertex_shader_layout   = shader->vertex_layout,
                                                                   .fragment_shader_layout = shader->fragment_layout,
                                                                   .width                  = renderer->window.width,
                                                                   .height                 = renderer->window.height};
    if (CreateVulkanGraphicsPipeline(&renderer->device, &pipeline_create_info, &renderer->graphics_pipeline))
    {
        return true;
    }

    renderer->components[renderer->component_count++] = RENDERER_GRAPHICS_PIPELINE_COMPONENT;
    return false;
}
