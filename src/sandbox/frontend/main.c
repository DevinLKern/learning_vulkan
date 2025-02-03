#include <engine/frontend/graphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/math.h>

int main()
{
    if (InitializeGraphicsLibraryFramework())
    {
        return 0;
    }

    Renderer renderer = Renderer_Create();
    if (renderer.component_count == 0)
    {
        UninitializeGraphicsLibraryFramework();
        return 0;
    }

    const ShaderCreateInfo shader_create_info = {.fragment_shader_path = "/home/dlk/Documents/code/learning_vulkan/compiled_shaders/fragment.spv",
                                                 .vertex_shader_path   = "/home/dlk/Documents/code/learning_vulkan/compiled_shaders/vertex.spv"};
    Shader shader                             = Shader_Create(&renderer, &shader_create_info);
    if (shader.descriptor_pool == VK_NULL_HANDLE)
    {
        Shader_Cleanup(&renderer, &shader);
        Renderer_Cleanup(&renderer);
        UninitializeGraphicsLibraryFramework();
        return 0;
    }

    MemoryArena arena = MemoryArena_Create(Shader_CalculateRequiredBytes(&renderer, &shader));

    if (Shader_Initialize(&renderer, &shader, &arena))
    {
        Shader_Cleanup(&renderer, &shader);
        Renderer_Cleanup(&renderer);
        MemoryArena_Free(&arena);
        UninitializeGraphicsLibraryFramework();
        return 0;
    }

    if (Renderer_InitializeGraphicsPipeline(&renderer, &shader))
    {
        Shader_Cleanup(&renderer, &shader);
        Renderer_Cleanup(&renderer);
        MemoryArena_Free(&arena);
        UninitializeGraphicsLibraryFramework();
        return 0;
    }

    const float vertices[]   = {0.5f, 0.5f, 0.1f, -0.5f, 0.5f, 0.1f, -0.5f, -0.5f, 0.1f};
    const uint32_t indices[] = {0, 1, 2};
    const Mat4f mvp[3]       = {Mat4f_Identity(), Mat4f_Identity(), Mat4f_Identity()};

    VulkanBuffer staging_buffer = {.memory = VK_NULL_HANDLE, .handle = VK_NULL_HANDLE};
    VulkanBuffer vertex_buffer  = {.memory = VK_NULL_HANDLE, .handle = VK_NULL_HANDLE};
    VulkanBuffer index_buffer   = {.memory = VK_NULL_HANDLE, .handle = VK_NULL_HANDLE};

    // initialize buffers
    {
        if (CreateVulkanStagingBuffer(&renderer.device, (sizeof(vertices) + sizeof(indices)), &staging_buffer))
        {
            ROSINA_LOG_ERROR("Could not create staging buffer");
            Shader_Cleanup(&renderer, &shader);
            Renderer_Cleanup(&renderer);
            MemoryArena_Free(&arena);
            UninitializeGraphicsLibraryFramework();
            return 0;
        }

        if (CreateVulkanVertexBuffer(&renderer.device, sizeof(vertices), &vertex_buffer))
        {
            ROSINA_LOG_ERROR("Could not create index buffer");
            Shader_Cleanup(&renderer, &shader);
            Renderer_Cleanup(&renderer);
            MemoryArena_Free(&arena);
            UninitializeGraphicsLibraryFramework();
            return 0;
        }

        if (CreateVulkanIndexBuffer(&renderer.device, sizeof(indices), &index_buffer))
        {
            ROSINA_LOG_ERROR("Could not create index buffer");
            Shader_Cleanup(&renderer, &shader);
            Renderer_Cleanup(&renderer);
            MemoryArena_Free(&arena);
            UninitializeGraphicsLibraryFramework();
            return 0;
        }
    }

    // start pre loop commands
    {
        VK_ERROR_HANDLE(vkResetCommandBuffer(renderer.primary_command_buffers[renderer.frame_index], 0), {
            Shader_Cleanup(&renderer, &shader);
            Renderer_Cleanup(&renderer);
            MemoryArena_Free(&arena);
            UninitializeGraphicsLibraryFramework();
            return 0;
        });
        const VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .pNext = NULL, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, .pInheritanceInfo = 0};
        VK_ERROR_HANDLE(vkBeginCommandBuffer(renderer.primary_command_buffers[renderer.frame_index], &begin_info), {
            Shader_Cleanup(&renderer, &shader);
            Renderer_Cleanup(&renderer);
            MemoryArena_Free(&arena);
            UninitializeGraphicsLibraryFramework();
            return 0;
        });
    }

    // populate buffers
    {
        {
            void* data = NULL;
            VK_ERROR_HANDLE(vkMapMemory(renderer.device.handle, staging_buffer.memory, 0, sizeof(vertices), 0, &data), {
                Shader_Cleanup(&renderer, &shader);
                Renderer_Cleanup(&renderer);
                MemoryArena_Free(&arena);
                UninitializeGraphicsLibraryFramework();
                return 0;
            });
            memcpy(data, vertices, sizeof(vertices));
            vkUnmapMemory(renderer.device.handle, staging_buffer.memory);

            const VkBufferCopy2 regions[] = {
                {.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2, .pNext = NULL, .srcOffset = 0, .dstOffset = 0, .size = sizeof(vertices)}};
            const VkCopyBufferInfo2 copy_info = {.sType       = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
                                                 .pNext       = NULL,
                                                 .srcBuffer   = staging_buffer.handle,
                                                 .dstBuffer   = vertex_buffer.handle,
                                                 .regionCount = sizeof(regions) / sizeof(VkBufferCopy2),
                                                 .pRegions    = regions};
            vkCmdCopyBuffer2(renderer.primary_command_buffers[renderer.frame_index], &copy_info);
        }
        {
            void* data = NULL;
            VK_ERROR_HANDLE(vkMapMemory(renderer.device.handle, staging_buffer.memory, sizeof(vertices), sizeof(indices), 0, &data), {
                Shader_Cleanup(&renderer, &shader);
                Renderer_Cleanup(&renderer);
                MemoryArena_Free(&arena);
                UninitializeGraphicsLibraryFramework();
                return 0;
            });
            memcpy(data, indices, sizeof(indices));
            vkUnmapMemory(renderer.device.handle, staging_buffer.memory);

            const VkBufferCopy2 regions[] = {
                {.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2, .pNext = NULL, .srcOffset = sizeof(vertices), .dstOffset = 0, .size = sizeof(indices)}};
            const VkCopyBufferInfo2 copy_info = {.sType       = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
                                                 .pNext       = NULL,
                                                 .srcBuffer   = staging_buffer.handle,
                                                 .dstBuffer   = index_buffer.handle,
                                                 .regionCount = sizeof(regions) / sizeof(VkBufferCopy2),
                                                 .pRegions    = regions};
            vkCmdCopyBuffer2(renderer.primary_command_buffers[renderer.frame_index], &copy_info);
        }
    }

    for (uint32_t i = 0; i < renderer.frame_count; i++)
    {
        void* data = NULL;
        VK_ERROR_HANDLE(vkMapMemory(renderer.device.handle, shader.uniform_buffers[i].memory, 0, sizeof(mvp), 0, &data), {
            Shader_Cleanup(&renderer, &shader);
            Renderer_Cleanup(&renderer);
            MemoryArena_Free(&arena);
            UninitializeGraphicsLibraryFramework();
            return 0;
        });
        memcpy(data, mvp, sizeof(mvp));
        vkUnmapMemory(renderer.device.handle, shader.uniform_buffers[i].memory);

        const VkDescriptorBufferInfo buffer_info = {
            .buffer = shader.uniform_buffers[i].handle,
            .offset = 0,
            .range  = VK_WHOLE_SIZE,
        };
        const VkWriteDescriptorSet descriptor_writes[] = {{.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                                           .pNext            = NULL,
                                                           .dstSet           = shader.descriptor_sets[i],
                                                           .dstBinding       = 0,
                                                           .dstArrayElement  = 0,
                                                           .descriptorCount  = 1,
                                                           .descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                           .pImageInfo       = NULL,
                                                           .pBufferInfo      = &buffer_info,
                                                           .pTexelBufferView = NULL}};
        vkUpdateDescriptorSets(renderer.device.handle, sizeof(descriptor_writes) / sizeof(VkWriteDescriptorSet), descriptor_writes, 0, NULL);
    }

    {
        VK_ERROR_HANDLE(vkEndCommandBuffer(renderer.primary_command_buffers[renderer.frame_index]), {
            Shader_Cleanup(&renderer, &shader);
            Renderer_Cleanup(&renderer);
            MemoryArena_Free(&arena);
            UninitializeGraphicsLibraryFramework();
            return 0;
        });
    }
    {
        VK_ERROR_HANDLE(vkWaitForFences(renderer.device.handle, 1, &renderer.in_flight[renderer.frame_index], VK_TRUE, UINT64_MAX), {
            Shader_Cleanup(&renderer, &shader);
            Renderer_Cleanup(&renderer);
            MemoryArena_Free(&arena);
            UninitializeGraphicsLibraryFramework();
            return 0;
        });
        VK_ERROR_HANDLE(vkResetFences(renderer.device.handle, 1, &renderer.in_flight[renderer.frame_index]), {
            Shader_Cleanup(&renderer, &shader);
            Renderer_Cleanup(&renderer);
            MemoryArena_Free(&arena);
            UninitializeGraphicsLibraryFramework();
            return 0;
        });

        const VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        const VkSubmitInfo submit_info           = {.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                                    .waitSemaphoreCount   = 0,
                                                    .pWaitSemaphores      = NULL,
                                                    .pWaitDstStageMask    = wait_stages,
                                                    .commandBufferCount   = 1,
                                                    .pCommandBuffers      = renderer.primary_command_buffers + renderer.frame_index,
                                                    .signalSemaphoreCount = 0,
                                                    .pSignalSemaphores    = NULL};
        VK_ERROR_HANDLE(vkQueueSubmit(renderer.main_queue.handle, 1, &submit_info, renderer.in_flight[renderer.frame_index]), {
            Shader_Cleanup(&renderer, &shader);
            Renderer_Cleanup(&renderer);
            MemoryArena_Free(&arena);
            UninitializeGraphicsLibraryFramework();
            return 0;
        });
    }

    vkDeviceWaitIdle(renderer.device.handle);

    while (!glfwWindowShouldClose(renderer.window.window))
    {
        glfwPollEvents();

        if (Renderer_StartScene(&renderer))  // also binds graphics pipeline
            break;

        Shader_Bind(&renderer, &shader);

        const VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(renderer.primary_command_buffers[renderer.frame_index], 0, 1, &vertex_buffer.handle, offsets);
        vkCmdBindIndexBuffer(renderer.primary_command_buffers[renderer.frame_index], index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);

        // const RendererSubmitInfo submit_info = {
        //     .vertex_buffer = &vertex_buffer,
        //     .index_buffer = &index_buffer,
        //     .transofrm_matrix = &transform_matrix,
        //     .index_count = sizeof(indices) / sizeof(uint32_t)
        // };
        vkCmdDrawIndexed(renderer.primary_command_buffers[renderer.frame_index], sizeof(indices) / sizeof(uint32_t), 1, 0, 0, 0);

        if (Renderer_EndScene(&renderer)) break;
    }

    vkDeviceWaitIdle(renderer.device.handle);

    DestroyVulkanBuffer(&renderer.device, &index_buffer);
    DestroyVulkanBuffer(&renderer.device, &vertex_buffer);
    DestroyVulkanBuffer(&renderer.device, &staging_buffer);

    Shader_Cleanup(&renderer, &shader);
    Renderer_Cleanup(&renderer);
    MemoryArena_Free(&arena);
    UninitializeGraphicsLibraryFramework();

    return 0;
}