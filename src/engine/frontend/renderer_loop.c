#include <engine/frontend/renderer.h>

#include <string.h>

#include <utility/log.h>


static inline bool StartFrame(RendererContext ctx [static 1]) {
    VK_ERROR_RETURN(vkWaitForFences(ctx->device.handle, 1, ctx->in_flight + ctx->frame_index, VK_TRUE, UINT64_MAX), true);
    VK_ERROR_RETURN(vkResetFences(ctx->device.handle, 1, ctx->in_flight + ctx->frame_index), true);
    
    const VkAcquireNextImageInfoKHR acquire_info = {
        .sType      = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
        .pNext      = NULL,
        .swapchain  = ctx->swapchain.handle,
        .timeout    = UINT64_MAX,
        .semaphore  = ctx->image_available[ctx->frame_index],
        .fence      = ctx->in_flight[ctx->frame_index],
        .deviceMask = 1
    };
    VK_ERROR_RETURN(vkAcquireNextImage2KHR(ctx->device.handle, &acquire_info, &ctx->image_index), true);

    VK_ERROR_RETURN(vkResetCommandBuffer(ctx->primary_command_buffers[ctx->frame_index], 0), true);
    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = 0
    };
    VK_ERROR_RETURN(vkBeginCommandBuffer(ctx->primary_command_buffers[ctx->frame_index], &begin_info), true);

    return false;
}

static inline void StartRenderPass(const RendererContext ctx [static 1]) {
    const VkClearValue clear_values [] = {
        { .color = { .float32 = { 0.0f, 0.0f, 0.0f, 0.0f } } },
        { .depthStencil = {1.0f, 0} }
    };

    const VkRenderPassBeginInfo render_pass_info = {
        .sType      = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext      = NULL,
        .renderPass = ctx->render_pass.handle,
        .framebuffer = ctx->framebuffers[ctx->image_index],
        .renderArea = {
            .offset = { 0, 0 },
            .extent = ctx->swapchain.extent
        },
        .clearValueCount = sizeof(clear_values) / sizeof(VkClearValue),
        .pClearValues = clear_values
    };
    vkCmdBeginRenderPass(ctx->primary_command_buffers[ctx->frame_index], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    const VkViewport viewport = {
        .x          = 0.0f,
        .y          = 0.0f,
        .width      = (float)ctx->swapchain.extent.width,
        .height     = (float)ctx->swapchain.extent.height,
        .minDepth   = 0.0f,
        .maxDepth   = 1.0f
    };
    vkCmdSetViewport(ctx->primary_command_buffers[ctx->frame_index], 0, 1, &viewport);

    const VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = ctx->swapchain.extent
    };
    vkCmdSetScissor(ctx->primary_command_buffers[ctx->frame_index], 0, 1, &scissor);
}

static inline void EndRenderPass(const RendererContext ctx [static 1]) {
    vkCmdEndRenderPass(ctx->primary_command_buffers[ctx->frame_index]);
}

static inline bool EndFrame(const RendererContext ctx [static 1]) {
    VK_ERROR_RETURN(vkEndCommandBuffer(ctx->primary_command_buffers[ctx->frame_index]), true);

    VK_ERROR_RETURN(vkWaitForFences(ctx->device.handle, 1, ctx->in_flight + ctx->frame_index, VK_TRUE, UINT64_MAX), true);
    VK_ERROR_RETURN(vkResetFences(ctx->device.handle, 1, ctx->in_flight + ctx->frame_index), true);

    const VkPipelineStageFlags wait_stages [] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    const VkSubmitInfo submit_info = {
        .sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount     = 1,
        .pWaitSemaphores        = ctx->image_available + ctx->frame_index,
        .pWaitDstStageMask      = wait_stages,
        .commandBufferCount     = 1,
        .pCommandBuffers        = ctx->primary_command_buffers + ctx->frame_index, 
        .signalSemaphoreCount   = 1,
        .pSignalSemaphores      = ctx->render_finished + ctx->frame_index
    };
    VK_ERROR_RETURN(vkQueueSubmit(ctx->main_queue.handle, 1, &submit_info, ctx->in_flight[ctx->frame_index]), true); 

    const VkPresentInfoKHR present_info = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = ctx->render_finished + ctx->frame_index,
        .swapchainCount     = 1,
        .pSwapchains        = &ctx->swapchain.handle,
        .pImageIndices      = &ctx->image_index
    };

    VK_ERROR_RETURN(vkQueuePresentKHR(ctx->main_queue.handle, &present_info), true);

    return false;
}

bool Renderer_Loop(Renderer renderer [static 1]) {
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    {
        const VkDescriptorPoolSize pool_sizes [] = {
            { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1 }
        };
        const VkDescriptorPoolCreateInfo pool_create_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .maxSets = 2,
            .poolSizeCount = sizeof(pool_sizes) / sizeof(VkDescriptorPoolSize),
            .pPoolSizes = pool_sizes
        };
        VK_ERROR_RETURN(vkCreateDescriptorPool(renderer->context.device.handle, &pool_create_info, NULL, &descriptor_pool), true);
    }

    VkDescriptorSet descriptor_sets [] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
    {
        const VkDescriptorSetLayout set_layouts [] = {
            renderer->context.graphics_pipeline.vertex_shader_layout,
            renderer->context.graphics_pipeline.fragment_shader_layout
        };
        const VkDescriptorSetAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = NULL,
            .descriptorPool = descriptor_pool,
            .descriptorSetCount = sizeof(set_layouts) / sizeof(VkDescriptorSetLayout),
            .pSetLayouts = set_layouts
        };
        VK_ERROR_RETURN(vkAllocateDescriptorSets(renderer->context.device.handle, &alloc_info, descriptor_sets), true);
    }

    const float vertices [] = { 
         0.5f,  0.5f, 0.1f, 
        -0.5f,  0.5f, 0.1f, 
        -0.5f, -0.5f, 0.1f 
    };
    const uint32_t indices [] = { 0, 1, 2 };
    const float mvp [3][4][4] = {{
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f }
    }, {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f }
    }, {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f }
    }};

    {
        VK_ERROR_RETURN(vkResetCommandBuffer(renderer->context.primary_command_buffers[renderer->context.frame_index], 0), true);
        const VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = NULL,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = 0
        };
        VK_ERROR_RETURN(vkBeginCommandBuffer(renderer->context.primary_command_buffers[renderer->context.frame_index], &begin_info), true);
    }

    VulkanBuffer staging_buffer = {
        .memory = VK_NULL_HANDLE,
        .handle = VK_NULL_HANDLE
    };
    if (CreateVulkanStagingBuffer(&renderer->context.device, (sizeof(vertices) + sizeof(indices)), &staging_buffer)) {
        ROSINA_LOG_ERROR("Could not create staging buffer");
        return true;
    }

    VulkanBuffer vertex_buffer = {
        .memory = VK_NULL_HANDLE,
        .handle = VK_NULL_HANDLE
    };
    {
        if (CreateVulkanVertexBuffer(&renderer->context.device, sizeof(vertices), &vertex_buffer)) {
            ROSINA_LOG_ERROR("Could not create vertex buffer");
            return true;
        }

        void* data = NULL;
        vkMapMemory(renderer->context.device.handle, staging_buffer.memory, 0, sizeof(vertices), 0, &data);
        memcpy(data, vertices, sizeof(vertices));
        vkUnmapMemory(renderer->context.device.handle, staging_buffer.memory);

        const VkBufferCopy2 copy_stuff = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
            .pNext = NULL,
            .srcOffset = 0,
            .dstOffset = 0,
            .size = sizeof(vertices)
        };
        const VkCopyBufferInfo2 copy_info = {
            .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
            .pNext = NULL,
            .srcBuffer = staging_buffer.handle,
            .dstBuffer = vertex_buffer.handle,
            .regionCount = 1,
            .pRegions = &copy_stuff
        };
        vkCmdCopyBuffer2(renderer->context.primary_command_buffers[renderer->context.frame_index], &copy_info);
    }

    VulkanBuffer index_buffer = {
        .memory = VK_NULL_HANDLE,
        .handle = VK_NULL_HANDLE
    };
    {
        if (CreateVulkanIndexBuffer(&renderer->context.device, sizeof(indices), &index_buffer)) {
            DestroyVulkanBuffer(&renderer->context.device, &staging_buffer);
            DestroyVulkanBuffer(&renderer->context.device, &vertex_buffer);
            ROSINA_LOG_ERROR("Could not create index buffer");
            return true;
        }

        void* data = NULL;
        vkMapMemory(renderer->context.device.handle, staging_buffer.memory, sizeof(vertices), sizeof(indices), 0, &data);
        memcpy(data, indices, sizeof(indices));
        vkUnmapMemory(renderer->context.device.handle, staging_buffer.memory);

        const VkBufferCopy2 copy_stuff = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
            .pNext = NULL,
            .srcOffset = sizeof(vertices),
            .dstOffset = 0,
            .size = sizeof(indices)
        };
        const VkCopyBufferInfo2 copy_info = {
            .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
            .pNext = NULL,
            .srcBuffer = staging_buffer.handle,
            .dstBuffer = index_buffer.handle,
            .regionCount = 1,
            .pRegions = &copy_stuff
        };
        vkCmdCopyBuffer2(renderer->context.primary_command_buffers[renderer->context.frame_index], &copy_info);
    }

    VulkanBuffer ubo_buffer = {
        .handle = VK_NULL_HANDLE,
        .memory = VK_NULL_HANDLE
    };
    {
        if (CreateVulkanUniformBuffer(&renderer->context.device, sizeof(mvp), &ubo_buffer)) {
            return true;
        }

        void* ubo_buffer_write_location = NULL;
        vkMapMemory(
            renderer->context.device.handle, 
            ubo_buffer.memory, 
            0, 
            sizeof(mvp), 
            0, 
            &ubo_buffer_write_location
        );

        memcpy(ubo_buffer_write_location, mvp, sizeof(mvp));

        const VkDescriptorBufferInfo buffer_info = {
            .buffer = ubo_buffer.handle,
            .offset = 0,
            .range  = VK_WHOLE_SIZE,
        };
        const VkWriteDescriptorSet descriptor_writes [] = {{
            .sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext              = NULL,
            .dstSet             = descriptor_sets[0],
            .dstBinding         = 0,
            .dstArrayElement    = 0,
            .descriptorCount    = 1,
            .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo         = NULL,
            .pBufferInfo        = &buffer_info,
            .pTexelBufferView   = NULL
        }};
        vkUpdateDescriptorSets(
            renderer->context.device.handle, 
            sizeof(descriptor_writes) / sizeof(VkWriteDescriptorSet), 
            descriptor_writes, 
            0, 
            NULL
        );
    }

    VK_ERROR_RETURN(vkEndCommandBuffer(renderer->context.primary_command_buffers[renderer->context.frame_index]), true);

    {
        VK_ERROR_RETURN(vkWaitForFences(
            renderer->context.device.handle, 
            1, 
            &renderer->context.in_flight[renderer->context.frame_index], 
            VK_TRUE, 
            UINT64_MAX
        ), true);
        VK_ERROR_RETURN(vkResetFences(renderer->context.device.handle, 1, &renderer->context.in_flight[renderer->context.frame_index]), true);
        
        const VkPipelineStageFlags wait_stages [] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        const VkSubmitInfo submit_info = {
            .sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount     = 0,
            .pWaitSemaphores        = NULL,
            .pWaitDstStageMask      = wait_stages,
            .commandBufferCount     = 1,
            .pCommandBuffers        = renderer->context.primary_command_buffers + renderer->context.frame_index, 
            .signalSemaphoreCount   = 0,
            .pSignalSemaphores      = NULL
        };
        VK_ERROR_RETURN(vkQueueSubmit(
            renderer->context.main_queue.handle, 
            1, 
            &submit_info, 
            renderer->context.in_flight[renderer->context.frame_index]
        ), true); 
    }

    vkDeviceWaitIdle(renderer->context.device.handle);
    
    // for (uint32_t i = 0; i < 3 && !glfwWindowShouldClose(renderer->window.window); i++) {
    while (!glfwWindowShouldClose(renderer->window.window)) {
        glfwPollEvents();

        // ROSINA_LOG_INFO("marker 10");

        if (StartFrame(&renderer->context)) {
        //     DestroyVulkanBuffer(&renderer->context.device, &staging_buffer);
        //     DestroyVulkanBuffer(&renderer->context.device, &vertex_buffer);
        //     DestroyVulkanBuffer(&renderer->context.device, &index_buffer);
            return true;
        }

        // ROSINA_LOG_INFO("marker 20");

        StartRenderPass(&renderer->context);

        // ROSINA_LOG_INFO("marker 30");

        vkCmdBindDescriptorSets(
            renderer->context.primary_command_buffers[renderer->context.frame_index],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            renderer->context.graphics_pipeline.layout,
            0,
            sizeof(descriptor_sets) / sizeof(VkDescriptorSet),
            descriptor_sets,
            0,
            NULL
        );

        vkCmdBindPipeline(
            renderer->context.primary_command_buffers[renderer->context.frame_index], 
            VK_PIPELINE_BIND_POINT_GRAPHICS, 
            renderer->context.graphics_pipeline.handle
        );
        const VkDeviceSize offsets [] = { 0 };
        vkCmdBindVertexBuffers(
            renderer->context.primary_command_buffers[renderer->context.frame_index], 
            0, 
            1, 
            &vertex_buffer.handle, 
            offsets
        );
        vkCmdBindIndexBuffer(
            renderer->context.primary_command_buffers[renderer->context.frame_index], 
            index_buffer.handle, 
            0, 
            VK_INDEX_TYPE_UINT32
        );

        vkCmdDrawIndexed(
            renderer->context.primary_command_buffers[renderer->context.frame_index], 
            sizeof(indices) / sizeof(uint32_t), 
            1, 
            0, 
            0, 
            0
        );

        EndRenderPass(&renderer->context);

        if (EndFrame(&renderer->context)) {
        //     DestroyVulkanBuffer(&renderer->context.device, &staging_buffer);
        //     DestroyVulkanBuffer(&renderer->context.device, &vertex_buffer);
        //     DestroyVulkanBuffer(&renderer->context.device, &index_buffer);
            return true;
        }

        renderer->context.frame_index += 1;
        renderer->context.frame_index %= renderer->context.frame_count;
    }

    vkDeviceWaitIdle(renderer->context.device.handle);

    DestroyVulkanBuffer(&renderer->context.device, &staging_buffer);
    DestroyVulkanBuffer(&renderer->context.device, &vertex_buffer);
    DestroyVulkanBuffer(&renderer->context.device, &index_buffer);
    DestroyVulkanBuffer(&renderer->context.device, &ubo_buffer);

    //

    vkDestroyDescriptorPool(renderer->context.device.handle, descriptor_pool, NULL);
    
    return false;
}