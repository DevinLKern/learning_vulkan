#include <engine/frontend/renderer.h>

#include <assert.h>
#include <stdlib.h>

void RendererContext_Cleanup(RendererContext context [static 1]) {
	while (context->component_count > 0) {
		switch (context->components[--context->component_count]) {
		case RENDER_CONTEXT_DEVICE_COMPONENT:
			DestroyVulkanDevice(&context->device);
			break;
		case RENDER_CONTEXT_RENDER_PASS_COMPONENT:
			DestroyVulkanRenderPass(&context->device, &context->render_pass);
			break;
		case RENDER_CONTEXT_GRAPHICS_PIPELINE_COMPONENT:
			DestroyVulkanGraphicsPipeline(&context->device, &context->graphics_pipeline);
			break;
		case RENDER_CONTEXT_SWAPCHAIN_COMPONENT:
			DestroyVulkanSwapchain(&context->device, &context->swapchain);
			break;
		case RENDER_CONTEXT_IMAGE_AVAILABLE_COMPONENT:
			for (uint32_t i = 0; i < context->frame_count; i++) {
				vkDestroySemaphore(context->device.handle, context->image_available[i], NULL);
			}
			break;
		case RENDER_CONTEXT_RENDER_FINISHED_COMPONENT:
			for (uint32_t i = 0; i < context->frame_count; i++) {
				vkDestroySemaphore(context->device.handle, context->render_finished[i], NULL);
			}
			break;
		case RENDER_CONTEXT_FRAME_IN_FLIGHT_COMPONENT:
			for (uint32_t i = 0; i < context->frame_count; i++) {
				vkDestroyFence(context->device.handle, context->in_flight[i], NULL);
			}
			break;
		case RENDER_CONTEXT_DEPTH_IMAGES_COMPONENT:
			for (uint32_t i = 0; i < context->image_count; i++) {
				vkDestroyImage(context->device.handle, context->depth_images[i], NULL);
			}
			break;
		case RENDER_CONTEXT_DEPTH_IMAGE_MEMORIES_COMPONENT:
			for (uint32_t i = 0; i < context->image_count; i++) {
				vkFreeMemory(context->device.handle, context->depth_image_memories[i], NULL);
			}
			break;
		case RENDER_CONTEXT_SWAPCHAIN_IMAGE_VIEWS_COMPONENT:
			for (uint32_t i = 0; i < context->image_count; i++) {
				vkDestroyImageView(context->device.handle, context->swapchain_image_views[i], NULL);
			}
			break;
		case RENDER_CONTEXT_DEPTH_IMAGE_VIEWS_COMPONENT:
			for (uint32_t i = 0; i < context->image_count; i++) {
				vkDestroyImageView(context->device.handle, context->depth_image_views[i], NULL);
			}
			break;
		case RENDER_CONTEXT_FRAMEBUFFERS_COMPONENT:
			for (uint32_t i = 0; i < context->image_count; i++) {
				vkDestroyFramebuffer(context->device.handle, context->framebuffers[i], NULL);
			}
			break;
		case RENDER_CONTEXT_COMMAND_POOLS_COMPONENT:
			for (uint32_t i = 0; i < context->frame_count; i++) {
				vkDestroyCommandPool(context->device.handle, context->command_pools[i], NULL);
			}
			break;
		case RENDER_CONTEXT_PRIMARY_COMMAND_BUFFERS_COMPONENT:
			for (uint32_t i = 0; i < context->frame_count; i++) {
                    vkFreeCommandBuffers(context->device.handle, context->command_pools[i], 1, context->primary_command_buffers + i);
                }
			break;
		default:
			ROSINA_LOG_ERROR("Invalid context component value");
            assert(false);
			break;
		}
	}
}

static inline uint32_t CalculateFrameCount(const VulkanSwapchain swapchain [static 1]) {
    // Check VkSurfaceCapabilitiesKHR::maxImageCount ?
    return swapchain->present_mode == VK_PRESENT_MODE_FIFO_KHR ? 2 : 3; 
}

static inline uint32_t CalculateMaxImageCount(
    const VulkanDevice                  device                  [static 1],
    const VulkanSwapchain               swapchain               [static 1]
) {
    VkSurfaceCapabilitiesKHR capabilities = {};
    VK_ERROR_RETURN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical_device, device->surface, &capabilities), 0);

    if (capabilities.maxImageCount > 0) {
        return capabilities.maxImageCount;
    }

    uint32_t image_count = 0;
    VK_ERROR_HANDLE(vkGetSwapchainImagesKHR(device->handle, swapchain->handle, &image_count, NULL), {
        return 0;
    });

    return image_count + 4;
}

RendererContext RendererContext_Create(const RendererContextCreateInfo create_info [static 1]) {
	RendererContext context = {
        .component_count = 0,
        .components = {},
        .device = {
            .component_count = 0,
            .components = {},
            .instance = VK_NULL_HANDLE,
            .debug_messenger = VK_NULL_HANDLE,
            .surface = VK_NULL_HANDLE,
            .physical_device = VK_NULL_HANDLE,
            .handle = VK_NULL_HANDLE
        },
        .render_pass = {
            .depth_format = VK_FORMAT_UNDEFINED,
            .surface_format = {
                .format = VK_OBJECT_TYPE_UNKNOWN,
                .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
            },
            .handle = VK_NULL_HANDLE
        },
        .graphics_pipeline = {
            .component_count = 0,
            .components = {},
            .vertex_shader_layout = VK_NULL_HANDLE,
            .fragment_shader_layout = VK_NULL_HANDLE,
            .layout = VK_NULL_HANDLE,
            .handle = VK_NULL_HANDLE
        },
        .swapchain = {
            .present_mode = VK_PRESENT_MODE_FIFO_KHR,
            .extent = {
                .width = 0,
                .height = 0
            },
            .handle = VK_NULL_HANDLE
        },
        .image_count = 0,
        .swapchain_images = NULL,
        .depth_images = NULL,
        .depth_image_memories = NULL,
        .depth_image_views = NULL,
        .swapchain_image_views = NULL,
        .framebuffers = NULL,
        .main_queue = {
            .handle = VK_NULL_HANDLE,
            .family_index = UINT32_MAX,
            .queue_index = 0
        },
        .image_index = 0,
        .frame_index = 0
    };

	QueueCapabilityFlags queue_descriptions [] = { 
		QUEUE_CAPABLITY_FLAG_GRAPHICS_BIT | QUEUE_CAPABLITY_FLAG_PRESENT_BIT 
	};

	// Create contex->device
	{
		const VulkanDeviceCreateInfo device_create_info = {
			.window = create_info->window,
			.queue_description_count = sizeof(queue_descriptions) / sizeof(QueueCapabilityFlags),
			.queue_descriptions = queue_descriptions,
			.debug = create_info->debug
		};
		if (CreateVulkanDevice(&device_create_info, &context.device)) {
			ROSINA_LOG_ERROR("Failed to create VulkanDevice");
			return context;
		}

		context.components[context.component_count++] = RENDER_CONTEXT_DEVICE_COMPONENT;
	}

	// queue stuff
	{
		const FindQueueFamilyIndexInfo find_info = {
			.queue_capability_count = 1,
			.queue_capabilities = queue_descriptions
		};
		if (FindQueueFamilyIndices(&context.device, &find_info, &context.main_queue.family_index)) {
			ROSINA_LOG_ERROR("Could not find queue family indices");
			RendererContext_Cleanup(&context);
			return context;
		}
		const VkDeviceQueueInfo2 queue_info = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
			.pNext = NULL,
			.flags = 0,
			.queueFamilyIndex = context.main_queue.family_index,
			.queueIndex = 0
		};
		vkGetDeviceQueue2(context.device.handle, &queue_info, &context.main_queue.handle);
	}

	// Create contex->render_pass
	{
		if (CreateVulkanRenderPass(&context.device, &context.render_pass)) {
			ROSINA_LOG_ERROR("Could not create render pass");
			RendererContext_Cleanup(&context);
			return context;
		}
		context.components[context.component_count++] = RENDER_CONTEXT_RENDER_PASS_COMPONENT;
	}

	// Create context->graphics_pipeline
	{
		const VulkanGraphicsPipelineCreateInfo pipeline_create_info = {
			.vertex_shader_path = "/home/dlk/Documents/code/tbd/compiled_shaders/vertex.spv",
			.fragment_shader_path = "/home/dlk/Documents/code/tbd/compiled_shaders/fragment.spv",
			.render_pass = &context.render_pass,
			.vertex_input_description_count = 0,
			.vertex_input_descriptions = NULL,
			.width = create_info->window->height,
			.height = create_info->window->height
		};
		if (CreateVulkanGraphicsPipeline(&context.device, &pipeline_create_info, &context.graphics_pipeline)) {
			ROSINA_LOG_ERROR("Could not create graphics pipeline");
			RendererContext_Cleanup(&context);
			return context;
		}

		context.components[context.component_count++] = RENDER_CONTEXT_GRAPHICS_PIPELINE_COMPONENT;
	}

	// Create context->swapchain
	{
		const VulkanSwapchainCreateInfo swapchain_create_info = {
			.queue_family_indices = &context.main_queue.family_index,
			.queue_family_index_count = 1,
			.width = create_info->window->width,
			.height = create_info->window->height,
			.render_pass = &context.render_pass
		};
		if (CreateVulkanSwapchain(&context.device, &swapchain_create_info, &context.swapchain)) {
			ROSINA_LOG_ERROR("Failed to create VulkanSwapchain");
			RendererContext_Cleanup(&context);
			return context;
		}
		context.components[context.component_count++] = RENDER_CONTEXT_SWAPCHAIN_COMPONENT;
	}

	// Calculate frame_count and image_capacity
	{
		context.frame_count = CalculateFrameCount(&context.swapchain);
		context.image_capacity = CalculateMaxImageCount(&context.device, &context.swapchain);
		if (context.image_capacity == 0) {
			RendererContext_Cleanup(&context);
			return context;
		}
	}

	return context;
}

uint64_t RendererContex_CalculateRequiredBytes(const RendererContext context [static 1]) {
	return (context->frame_count * (
		sizeof(VkSemaphore) +		// context->image_available
		sizeof(VkCommandPool) +		// context->command_pools
		sizeof(VkCommandBuffer)	+	// context->main_buffers
		sizeof(VkSemaphore) +		// context->render_finished
		sizeof(VkFence)  			// context->in_flight
	)) + (context->image_capacity * (
        sizeof(VkImage) +           // context->swapchain_images 
        sizeof(VkImage) +           // context->depth_images 
        sizeof(VkDeviceMemory) +    // context->depth_image_memories
		sizeof(VkImageView) +       // context->swapchain_image_views 
        sizeof(VkImageView) +       // context->depth_image_views
		sizeof(VkFramebuffer) 		// context->framebuffers
    ));
}

bool RendererContext_Initialize(RendererContext context [static 1], const uint32_t width, const uint32_t height, MemoryArena arena [static 1]) {
	// get swapchain images
	{
		context->swapchain_images = MemoryArena_Allocate(arena, context->image_capacity * sizeof(VkImage));

		VK_ERROR_HANDLE(vkGetSwapchainImagesKHR(
			context->device.handle, 
			context->swapchain.handle, 
			&context->image_count, 
			NULL
		), {
    	    RendererContext_Cleanup(context);
    	    return true;
    	});

		if (context->image_count > context->image_capacity) {
			ROSINA_LOG_ERROR("I hate my life");
			RendererContext_Cleanup(context);
    	    return true;
		}

		VK_ERROR_HANDLE(vkGetSwapchainImagesKHR(
			context->device.handle, 
			context->swapchain.handle, 
			&context->image_count, 
			context->swapchain_images
		), {
    	    RendererContext_Cleanup(context);
    	    return true;
    	});
	}

    // create depth images
    {
		context->depth_images = MemoryArena_Allocate(arena, context->image_capacity * sizeof(VkImage));

        const VkImageCreateInfo image_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = context->render_pass.depth_format,
            .extent = {
                .width = context->swapchain.extent.width,
                .height = context->swapchain.extent.height,
                .depth = 1
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL, // Could be VK_IMAGE_TILING_LINEAR ?
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,  // Read the docs
            .pQueueFamilyIndices = NULL, // Read the docs
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };

        for (uint32_t i = 0; i < context->image_count; i++) {
            VK_ERROR_HANDLE(vkCreateImage(context->device.handle, &image_create_info, NULL, context->depth_images + i), {
                for (uint32_t j = 0; j < i; j++) {
                    vkDestroyImage(context->device.handle, context->depth_images[j], NULL);
                }
                RendererContext_Cleanup(context);
                return true;
            });
        }

		context->components[context->component_count++] = RENDER_CONTEXT_DEPTH_IMAGES_COMPONENT;
    }

    // create depth image memories
    {
		context->depth_image_memories = MemoryArena_Allocate(arena, context->image_capacity * sizeof(VkDeviceMemory));

		if (CreateVulkanImageMemories(&context->device, context->image_count, context->depth_images, context->depth_image_memories)) {
			ROSINA_LOG_ERROR("Failed to create depth image memories");
			RendererContext_Cleanup(context);
    		return true;
		}

		context->components[context->component_count++] = RENDER_CONTEXT_DEPTH_IMAGE_MEMORIES_COMPONENT;
	}

	// create depth image views
	{
		context->depth_image_views = MemoryArena_Allocate(arena, context->image_capacity * sizeof(VkImageView));

    	for (uint32_t i = 0; i < context->image_count; i++) {
			const VkImageViewCreateInfo image_view_create_info = {
				.sType 		= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext 		= NULL,
				.flags 		= 0,
				.image 		= context->depth_images[i],
				.viewType 	= VK_IMAGE_VIEW_TYPE_2D,
				.format 	= context->render_pass.depth_format,
				.components = {
					.r 		= VK_COMPONENT_SWIZZLE_IDENTITY,
					.g 		= VK_COMPONENT_SWIZZLE_IDENTITY,
					.b 		= VK_COMPONENT_SWIZZLE_IDENTITY,
					.a 		= VK_COMPONENT_SWIZZLE_IDENTITY
				},
				.subresourceRange 	= {
      			  	.aspectMask 	= VK_IMAGE_ASPECT_DEPTH_BIT,
      			  	.baseMipLevel 	= 0,
      			  	.levelCount 	= 1,
      			  	.baseArrayLayer = 0,
      			  	.layerCount 	= 1
      			}
			};
			VK_ERROR_HANDLE(vkCreateImageView(context->device.handle, &image_view_create_info, NULL, context->depth_image_views + i), {
				for (uint32_t j = 0; j < i; j++) {
        	        vkDestroyImageView(context->device.handle, context->depth_image_views[i], NULL);
        	    }
				RendererContext_Cleanup(context);
				return true;
			});
		}

		context->components[context->component_count++] = RENDER_CONTEXT_DEPTH_IMAGE_VIEWS_COMPONENT;
	}

	// create swapchain image views
	{
		context->swapchain_image_views = MemoryArena_Allocate(arena, context->image_capacity * sizeof(VkImageView));

		VkImageViewCreateInfo image_view_create_info = {
			.sType 		= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext 		= NULL,
			.flags 		= 0,
			.image 		= VK_NULL_HANDLE,
			.viewType 	= VK_IMAGE_VIEW_TYPE_2D,
			.format 	= context->render_pass.surface_format.format,
			.components = { 
      		  	.r = VK_COMPONENT_SWIZZLE_IDENTITY, 
      		  	.g = VK_COMPONENT_SWIZZLE_IDENTITY, 
      		  	.b = VK_COMPONENT_SWIZZLE_IDENTITY,
      		  	.a = VK_COMPONENT_SWIZZLE_IDENTITY
      		},
			.subresourceRange = {
      		  	.aspectMask 	= VK_IMAGE_ASPECT_COLOR_BIT,
      		  	.baseMipLevel 	= 0,
      		  	.levelCount 	= 1,
      		  	.baseArrayLayer = 0,
      		  	.layerCount 	= 1
      		}
		};
		const VulkanImageViewsCreateInfo image_views_create_info = {
			.image_count = context->image_count,
			.images = context->swapchain_images,
			.vk_image_create_info = &image_view_create_info,
		};
		if (CreateVulkanImageViews(&context->device, &image_views_create_info, context->swapchain_image_views)) {
			ROSINA_LOG_ERROR("Failed to create swapchain image views");
			RendererContext_Cleanup(context);
    		return true;
		}

		context->components[context->component_count++] = RENDER_CONTEXT_SWAPCHAIN_IMAGE_VIEWS_COMPONENT;
	}

	// create framebuffers
	{
		context->framebuffers = MemoryArena_Allocate(arena, context->image_capacity * sizeof(VkFramebuffer));

		for (uint32_t i = 0; i < context->image_count; i++) {
			const VkImageView attachments [] = {
				context->swapchain_image_views[i],
				context->depth_image_views[i]
			};
			const VkFramebufferCreateInfo framebuffer_create_info = {
				.sType 				= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.pNext 				= NULL,
				.flags 				= 0,
				.renderPass 		= context->render_pass.handle,
				.attachmentCount 	= sizeof(attachments) / sizeof(VkImageView),
				.pAttachments 		= attachments,
				.width 				= width,
				.height 			= height,
				.layers 			= 1
			};
			VK_ERROR_HANDLE(vkCreateFramebuffer(context->device.handle, &framebuffer_create_info, NULL, context->framebuffers + i), {
				for (uint32_t j = 0; j < i; j++) {
                    vkDestroyFramebuffer(context->device.handle, context->framebuffers[j], NULL);
                }
				RendererContext_Cleanup(context);
				return true;
			});
		}

		context->components[context->component_count++] = RENDER_CONTEXT_FRAMEBUFFERS_COMPONENT;
	}

	// create in flight fences
	{
		context->in_flight = MemoryArena_Allocate(arena, context->frame_count * sizeof(VkFence));

		const VkFenceCreateInfo fence_create_info = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = NULL,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT // VK_FENCE_CREATE_SIGNALED_BIT
		};
		for (uint32_t i = 0; i < context->frame_count; i++) {
			VK_ERROR_HANDLE(vkCreateFence(context->device.handle, &fence_create_info, NULL, context->in_flight + i), {
				for (uint32_t j = 0; j < i; j++) {
                    vkDestroyFence(context->device.handle, context->in_flight[j], NULL);
                }
				RendererContext_Cleanup(context);
				return true;
			});
		}

		context->components[context->component_count++] = RENDER_CONTEXT_FRAME_IN_FLIGHT_COMPONENT;
	}

	// create image available semaphores
	{
		context->image_available = MemoryArena_Allocate(arena, context->frame_count * sizeof(VkSemaphore));

		const VkSemaphoreCreateInfo semaphore_create_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0
		};
		for (uint32_t i = 0; i < context->frame_count; i++) {
			VK_ERROR_HANDLE(vkCreateSemaphore(context->device.handle, &semaphore_create_info, NULL, context->image_available + i), {
				for (uint32_t j = 0; j < i; j++) {
                    vkDestroySemaphore(context->device.handle, context->image_available[j], NULL);
                }
				RendererContext_Cleanup(context);
				return true;
			});
		}

		context->components[context->component_count++] = RENDER_CONTEXT_IMAGE_AVAILABLE_COMPONENT;
	}

	// create render finished semaphores
	{
		context->render_finished = MemoryArena_Allocate(arena, context->frame_count * sizeof(VkSemaphore));

		const VkSemaphoreCreateInfo semaphore_create_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0
		};
		for (uint32_t i = 0; i < context->frame_count; i++) {
			VK_ERROR_HANDLE(vkCreateSemaphore(context->device.handle, &semaphore_create_info, NULL, context->render_finished + i), {
				for (uint32_t j = 0; j < i; j++) {
                    vkDestroySemaphore(context->device.handle, context->render_finished[j], NULL);
                }
				RendererContext_Cleanup(context);
				return true;
			});
		}

		context->components[context->component_count++] = RENDER_CONTEXT_RENDER_FINISHED_COMPONENT;
	}

	// create command pools
	{
		context->command_pools = MemoryArena_Allocate(arena, context->frame_count * sizeof(VkCommandPool));

		const VkCommandPoolCreateInfo pool_create_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = NULL,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = context->main_queue.family_index
		};
		for (uint32_t i = 0; i < context->frame_count; i++) {
			VK_ERROR_HANDLE(vkCreateCommandPool(context->device.handle, &pool_create_info, NULL, context->command_pools + i), {
				for (uint32_t j = 0; j < i; j++) {
                    vkDestroyCommandPool(context->device.handle, context->command_pools[j], NULL);
                }
				RendererContext_Cleanup(context);
				return true;
			});
		}

		context->components[context->component_count++] = RENDER_CONTEXT_COMMAND_POOLS_COMPONENT;
	}

	// primary command buffers
	{
		context->primary_command_buffers = MemoryArena_Allocate(arena, context->frame_count * sizeof(VkCommandBuffer));

		for (uint32_t i = 0; i < context->frame_count; i++) {
			const VkCommandBufferAllocateInfo alloc_info = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.pNext = NULL,
				.commandPool = context->command_pools[i],
				.level = 0,
				.commandBufferCount = 1
			};
			VK_ERROR_HANDLE(vkAllocateCommandBuffers(context->device.handle, &alloc_info, context->primary_command_buffers + i), {
				for (uint32_t j = 0; j < i; j++) {
                    vkFreeCommandBuffers(context->device.handle, context->command_pools[j], 1, context->primary_command_buffers + j);
                }
				RendererContext_Cleanup(context);
				return true;
			});
		}
	}
	
	return false;
}

Renderer Renderer_Create() {
	Renderer renderer = {
        .component_count = 0,
        .components = {},
    };

	{
		const GLFWWindowCreateInfo window_create_info = {
    	    .width = 1000,
    	    .height = 1000,
    	    .title = "GLFWWindow"
    	};
		renderer.window = GLFWWindow_Create(&window_create_info);
		if (renderer.window.window == NULL) {
			ROSINA_LOG_ERROR("Could not create renderer window!");
			return renderer;
		}

		renderer.components[renderer.component_count++] = RENDERER_WINDOW_COMPONENT;
	}

	{
		const RendererContextCreateInfo create_info = {
			.debug 	= true,
			.window = &renderer.window
		};
		renderer.context = RendererContext_Create(&create_info);
		if (renderer.context.component_count == 0) {
			Renderer_Cleanup(&renderer);
			return renderer;
		}
		renderer.components[renderer.component_count++] = RENDERER_CONTEXT_COMPONENT;
	}

	return renderer;
}

uint64_t Renderer_CalculateRequiredBytes(const Renderer renderer [static 1]) {
	return RendererContex_CalculateRequiredBytes(&renderer->context);
}

bool Renderer_Initialize(Renderer renderer [static 1], MemoryArena arena [static 1]) {
	if (RendererContext_Initialize(&renderer->context, renderer->window.width, renderer->window.height, arena)) {
		return true;
	}

	return false;
}

void Renderer_Cleanup(Renderer renderer [static 1]) {
	while (renderer->component_count > 0) {
		switch (renderer->components[--renderer->component_count]) {
		case RENDERER_WINDOW_COMPONENT:
			GLFWWindow_Cleanup(&renderer->window);
			break;
		case RENDERER_CONTEXT_COMPONENT:
			RendererContext_Cleanup(&renderer->context);
			break;
		default:
			ROSINA_LOG_ERROR("Invalid renderer component value");
            assert(false);
		}
	}
}