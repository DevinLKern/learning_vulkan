#include <engine/graphics/renderer.h>

#include <string.h>
#include <utility/log.h>

static inline bool StartFrame(Renderer renderer[static 1])
{
    VK_ERROR_RETURN(vkWaitForFences(renderer->device.handle, 1, renderer->in_flight + renderer->frame_index, VK_TRUE, UINT64_MAX), true);
    VK_ERROR_RETURN(vkResetFences(renderer->device.handle, 1, renderer->in_flight + renderer->frame_index), true);

    const VkAcquireNextImageInfoKHR acquire_info = {
        .sType      = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
        .pNext      = NULL,
        .swapchain  = renderer->swapchain.handle,
        .timeout    = UINT64_MAX,
        .semaphore  = renderer->image_available[renderer->frame_index],
        .fence      = renderer->in_flight[renderer->frame_index],
        .deviceMask = 1
    };
    VK_ERROR_RETURN(vkAcquireNextImage2KHR(renderer->device.handle, &acquire_info, &renderer->image_index), true);

    VK_ERROR_RETURN(vkResetCommandBuffer(renderer->primary_command_buffers[renderer->frame_index], 0), true);
    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = NULL
    };
    VK_ERROR_RETURN(vkBeginCommandBuffer(renderer->primary_command_buffers[renderer->frame_index], &begin_info), true);

    return false;
}

static inline void StartRenderPass(const Renderer renderer[static 1])
{
    const VkClearValue clear_values[] = {{.color = {.float32 = {0.0f, 0.0f, 0.0f, 0.0f}}}, {.depthStencil = {1.0f, 0}}};

    const VkRenderPassBeginInfo render_pass_info = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext           = NULL,
        .renderPass      = renderer->render_pass.handle,
        .framebuffer     = renderer->framebuffers[renderer->image_index],
        .renderArea      = {.offset = {0, 0}, .extent = renderer->swapchain.extent},
        .clearValueCount = sizeof(clear_values) / sizeof(VkClearValue),
        .pClearValues    = clear_values
    };
    vkCmdBeginRenderPass(renderer->primary_command_buffers[renderer->frame_index], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    const VkViewport viewport = {
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = (float)renderer->swapchain.extent.width,
        .height   = (float)renderer->swapchain.extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(renderer->primary_command_buffers[renderer->frame_index], 0, 1, &viewport);

    const VkRect2D scissor = {.offset = {0, 0}, .extent = renderer->swapchain.extent};
    vkCmdSetScissor(renderer->primary_command_buffers[renderer->frame_index], 0, 1, &scissor);
}

bool Renderer_StartScene(Renderer renderer[static 1])
{
    if (StartFrame(renderer)) return true;

    StartRenderPass(renderer);

    vkCmdBindPipeline(renderer->primary_command_buffers[renderer->frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->graphics_pipeline.handle);

    return false;
}

static inline void EndRenderPass(const Renderer renderer[static 1])
{
    vkCmdEndRenderPass(renderer->primary_command_buffers[renderer->frame_index]);
}

static inline bool EndFrame(const Renderer renderer[static 1])
{
    VK_ERROR_RETURN(vkEndCommandBuffer(renderer->primary_command_buffers[renderer->frame_index]), true);

    VK_ERROR_RETURN(vkWaitForFences(renderer->device.handle, 1, renderer->in_flight + renderer->frame_index, VK_TRUE, UINT64_MAX), true);
    VK_ERROR_RETURN(vkResetFences(renderer->device.handle, 1, renderer->in_flight + renderer->frame_index), true);

    const VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    const VkSubmitInfo submit_info = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = renderer->image_available + renderer->frame_index,
        .pWaitDstStageMask    = wait_stages,
        .commandBufferCount   = 1,
        .pCommandBuffers      = renderer->primary_command_buffers + renderer->frame_index,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = renderer->render_finished + renderer->frame_index
    };
    VK_ERROR_RETURN(vkQueueSubmit(renderer->device.graphics_queue.handle, 1, &submit_info, renderer->in_flight[renderer->frame_index]), true);

    const VkPresentInfoKHR present_info = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = renderer->render_finished + renderer->frame_index,
        .swapchainCount     = 1,
        .pSwapchains        = &renderer->swapchain.handle,
        .pImageIndices      = &renderer->image_index
    };

    VK_ERROR_RETURN(vkQueuePresentKHR(renderer->device.present_queue.handle, &present_info), true);

    return false;
}

bool Renderer_EndScene(Renderer renderer[static 1])
{
    EndRenderPass(renderer);

    if (EndFrame(renderer)) return true;

    renderer->frame_index += 1;
    renderer->frame_index %= renderer->frame_count;

    return false;
}