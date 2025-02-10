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

    const float vertices[]   = {0.5f, 0.5f, 0.1f, -0.5f, 0.5f, 0.1f, -0.5f, -0.5f, 0.1f};
    const uint32_t indices[] = {0, 1, 2};
    Mat4f mvp[3]             = {Mat4f_Identity(), Mat4f_Identity(), Mat4f_Identity()};

    VertexBufferObject vbo = {.size = sizeof(vertices), .offset = UINT64_MAX};
    IndexBufferObject ibo  = {.size = sizeof(indices), .offset = UINT64_MAX};

    MemoryArena arena;
    BufferMemory memory;
    Shader shader;
    {
        arena = MemoryArena_Create(Shader_CalculateRequiredBytes(&renderer));

        {
            const ShaderCreateInfo shader_create_info = {.fragment_shader_path = "/home/dlk/Documents/code/learning_vulkan/compiled_shaders/fragment.spv",
                                                         .vertex_shader_path   = "/home/dlk/Documents/code/learning_vulkan/compiled_shaders/vertex.spv",
                                                         .arena                = &arena,
                                                         .memory               = &memory};
            shader                                    = Shader_Create(&renderer, &shader_create_info);
            if (shader.descriptor_pool == VK_NULL_HANDLE)
            {
                vkDeviceWaitIdle(renderer.device.handle);
                Shader_Cleanup(&renderer, &shader);
                Renderer_Cleanup(&renderer);
                UninitializeGraphicsLibraryFramework();
                return 0;
            }
        }

        {
            const BufferInfo info = {
                .vbo_count = 1, .vbos = &vbo, .ibo_count = 1, .ibos = &ibo, .uniform_buffer_count = renderer.frame_count, .uniform_buffers = shader.ubos};

            memory = BufferMemory_Create(&renderer, &info);
            if (memory.handle == VK_NULL_HANDLE)
            {
                vkDeviceWaitIdle(renderer.device.handle);
                Shader_Cleanup(&renderer, &shader);
                Renderer_Cleanup(&renderer);
                MemoryArena_Free(&arena);
                UninitializeGraphicsLibraryFramework();
            }
        }
    }

    if (Renderer_InitializeGraphicsPipeline(&renderer, &shader))
    {
        vkDeviceWaitIdle(renderer.device.handle);
        Shader_Cleanup(&renderer, &shader);
        BufferMemory_Cleanup(&renderer, &memory);
        Renderer_Cleanup(&renderer);
        MemoryArena_Free(&arena);
        UninitializeGraphicsLibraryFramework();
        return 0;
    }

    StagingBuffer staging_buffer = StagingBuffer_Create(&renderer, sizeof(vertices) + sizeof(indices) + (sizeof(mvp) * renderer.frame_count));

    // start pre loop commands
    {
        VK_ERROR_HANDLE(vkResetCommandBuffer(renderer.primary_command_buffers[renderer.frame_index], 0), {
            vkDeviceWaitIdle(renderer.device.handle);
            Shader_Cleanup(&renderer, &shader);
            BufferMemory_Cleanup(&renderer, &memory);
            Renderer_Cleanup(&renderer);
            MemoryArena_Free(&arena);
            UninitializeGraphicsLibraryFramework();
            return 0;
        });
        const VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .pNext = NULL, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, .pInheritanceInfo = 0};
        VK_ERROR_HANDLE(vkBeginCommandBuffer(renderer.primary_command_buffers[renderer.frame_index], &begin_info), {
            vkDeviceWaitIdle(renderer.device.handle);
            Shader_Cleanup(&renderer, &shader);
            BufferMemory_Cleanup(&renderer, &memory);
            Renderer_Cleanup(&renderer);
            MemoryArena_Free(&arena);
            UninitializeGraphicsLibraryFramework();
            return 0;
        });
    }

    // populate buffers
    {
        // vertex buffer
        {
            void* data = NULL;
            VK_ERROR_HANDLE(vkMapMemory(renderer.device.handle, staging_buffer.memory, 0, sizeof(vertices), 0, &data), {
                vkDeviceWaitIdle(renderer.device.handle);
                Shader_Cleanup(&renderer, &shader);
                Renderer_Cleanup(&renderer);
                MemoryArena_Free(&arena);
                UninitializeGraphicsLibraryFramework();
                return 0;
            });
            memcpy(data, vertices, sizeof(vertices));
            vkUnmapMemory(renderer.device.handle, staging_buffer.memory);

            const VkBufferCopy2 regions[] = {
                {.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2, .pNext = NULL, .srcOffset = 0, .dstOffset = vbo.offset, .size = vbo.size}};
            const VkCopyBufferInfo2 copy_info = {.sType       = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
                                                 .pNext       = NULL,
                                                 .srcBuffer   = staging_buffer.handle,
                                                 .dstBuffer   = memory.vertex_buffer,
                                                 .regionCount = sizeof(regions) / sizeof(VkBufferCopy2),
                                                 .pRegions    = regions};
            vkCmdCopyBuffer2(renderer.primary_command_buffers[renderer.frame_index], &copy_info);
        }

        // index buffer
        {
            void* data = NULL;
            VK_ERROR_HANDLE(vkMapMemory(renderer.device.handle, staging_buffer.memory, sizeof(vertices), sizeof(indices), 0, &data), {
                vkDeviceWaitIdle(renderer.device.handle);
                Shader_Cleanup(&renderer, &shader);
                Renderer_Cleanup(&renderer);
                MemoryArena_Free(&arena);
                UninitializeGraphicsLibraryFramework();
                return 0;
            });
            memcpy(data, indices, sizeof(indices));
            vkUnmapMemory(renderer.device.handle, staging_buffer.memory);

            const VkBufferCopy2 regions[] = {
                {.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2, .pNext = NULL, .srcOffset = sizeof(vertices), .dstOffset = ibo.offset, .size = ibo.size}};
            const VkCopyBufferInfo2 copy_info = {.sType       = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
                                                 .pNext       = NULL,
                                                 .srcBuffer   = staging_buffer.handle,
                                                 .dstBuffer   = memory.index_buffer,
                                                 .regionCount = sizeof(regions) / sizeof(VkBufferCopy2),
                                                 .pRegions    = regions};
            vkCmdCopyBuffer2(renderer.primary_command_buffers[renderer.frame_index], &copy_info);
        }

        // uniform buffers
        for (uint32_t i = 0; i < renderer.frame_count; i++)
        {
            void* data = NULL;
            VK_ERROR_HANDLE(
                vkMapMemory(renderer.device.handle, staging_buffer.memory, sizeof(vertices) + sizeof(indices) + (sizeof(mvp) * i), sizeof(mvp), 0, &data), {
                    vkDeviceWaitIdle(renderer.device.handle);
                    Shader_Cleanup(&renderer, &shader);
                    Renderer_Cleanup(&renderer);
                    MemoryArena_Free(&arena);
                    UninitializeGraphicsLibraryFramework();
                    return 0;
                });
            memcpy(data, mvp, sizeof(mvp));
            vkUnmapMemory(renderer.device.handle, staging_buffer.memory);

            const VkBufferCopy2 regions[]     = {{.sType     = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
                                                  .pNext     = NULL,
                                                  .srcOffset = sizeof(vertices) + sizeof(indices) + (sizeof(mvp) * i),
                                                  .dstOffset = shader.ubos[i].offset,
                                                  .size      = shader.ubos[i].size}};
            const VkCopyBufferInfo2 copy_info = {.sType       = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
                                                 .pNext       = NULL,
                                                 .srcBuffer   = staging_buffer.handle,
                                                 .dstBuffer   = memory.uniform_buffer,
                                                 .regionCount = sizeof(regions) / sizeof(VkBufferCopy2),
                                                 .pRegions    = regions};
            vkCmdCopyBuffer2(renderer.primary_command_buffers[renderer.frame_index], &copy_info);
        }
    }

    {
        VK_ERROR_HANDLE(vkEndCommandBuffer(renderer.primary_command_buffers[renderer.frame_index]), {
            vkDeviceWaitIdle(renderer.device.handle);
            Shader_Cleanup(&renderer, &shader);
            Renderer_Cleanup(&renderer);
            MemoryArena_Free(&arena);
            UninitializeGraphicsLibraryFramework();
            return 0;
        });
    }
    {
        VK_ERROR_HANDLE(vkWaitForFences(renderer.device.handle, 1, &renderer.in_flight[renderer.frame_index], VK_TRUE, UINT64_MAX), {
            vkDeviceWaitIdle(renderer.device.handle);
            Shader_Cleanup(&renderer, &shader);
            Renderer_Cleanup(&renderer);
            MemoryArena_Free(&arena);
            UninitializeGraphicsLibraryFramework();
            return 0;
        });
        VK_ERROR_HANDLE(vkResetFences(renderer.device.handle, 1, &renderer.in_flight[renderer.frame_index]), {
            vkDeviceWaitIdle(renderer.device.handle);
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
            vkDeviceWaitIdle(renderer.device.handle);
            Shader_Cleanup(&renderer, &shader);
            Renderer_Cleanup(&renderer);
            MemoryArena_Free(&arena);
            UninitializeGraphicsLibraryFramework();
            return 0;
        });
    }

    for (uint32_t i = 0; i < renderer.frame_count; i++)
    {
        const VkDescriptorBufferInfo buffer_info = {
            .buffer = memory.uniform_buffer,
            .offset = shader.ubos[i].offset,
            .range  = shader.ubos[i].size,
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

    VK_ERROR_HANDLE(vkWaitForFences(renderer.device.handle, renderer.frame_count, renderer.in_flight, VK_TRUE, UINT64_MAX), {
        vkDeviceWaitIdle(renderer.device.handle);
        Shader_Cleanup(&renderer, &shader);
        Renderer_Cleanup(&renderer);
        MemoryArena_Free(&arena);
        UninitializeGraphicsLibraryFramework();
        return 0;
    });

    StagingBuffer_Cleanup(&renderer, &staging_buffer);
    vkDeviceWaitIdle(renderer.device.handle);

    while (!glfwWindowShouldClose(renderer.window.window))
    {
        glfwPollEvents();

        if (Renderer_StartScene(&renderer))  // also binds graphics pipeline
            break;

        Shader_Bind(&renderer, &shader);
        VertexBufferObject_Bind(&renderer, &memory, &vbo);
        IndexBufferObject_Bind(&renderer, &memory, &ibo);

        vkCmdDrawIndexed(renderer.primary_command_buffers[renderer.frame_index], sizeof(indices) / sizeof(uint32_t), 1, 0, 0, 0);

        if (Renderer_EndScene(&renderer)) break;
    }

    vkDeviceWaitIdle(renderer.device.handle);

    BufferMemory_Cleanup(&renderer, &memory);

    Shader_Cleanup(&renderer, &shader);
    Renderer_Cleanup(&renderer);
    MemoryArena_Free(&arena);
    UninitializeGraphicsLibraryFramework();

    return 0;
}