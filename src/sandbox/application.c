#include <assert.h>
#include <sandbox/application.h>

void Application_Cleanup(Application application[static 1])
{
    vkDeviceWaitIdle(application->renderer.device.handle);

    while (application->component_count > 0)
    {
        switch (application->components[--application->component_count])
        {
            case APPLICATION_LIBRARIES_COMPONENT:
                UninitializeGraphicsLibraryFramework();
                break;
            case APPLICATION_RENDERER_COMPONENT:
                Renderer_Cleanup(&application->renderer);
                break;
            case APPLICATION_MEMORY_ARENA_COMPONENT:
                MemoryArena_Free(&application->arena);
                break;
            case APPLICATION_SHADER_COMPONENT:
                Shader_Cleanup(&application->renderer, &application->shader);
                break;
            case APPLICATION_BUFFER_MEMORY_COMPONENT:
                BufferMemory_Cleanup(&application->renderer, &application->buffer_memory);
                break;
            default:
                ROSINA_LOG_ERROR("INVALID APPLICATION COMPONENT");
                assert(false);
        }
    }
}

Application Application_Create()
{
    Application application = {.component_count = 0, .components = {}};

    if (InitializeGraphicsLibraryFramework())
    {
        ROSINA_LOG_ERROR("Failed to initialize libraries");
        Application_Cleanup(&application);
        return application;
    }
    application.components[application.component_count++] = APPLICATION_LIBRARIES_COMPONENT;

    application.renderer = Renderer_Create();
    if (application.renderer.component_count == 0)
    {
        ROSINA_LOG_ERROR("Failed to create renderer");
        Application_Cleanup(&application);
        return application;
    }
    application.components[application.component_count++] = APPLICATION_RENDERER_COMPONENT;

    const float vertices[]   = {0.5f, 0.5f, 0.1f, -0.5f, 0.5f, 0.1f, -0.5f, -0.5f, 0.1f};
    const uint32_t indices[] = {0, 1, 2};
    Mat4f mvp[3]             = {Mat4f_Identity(), Mat4f_Identity(), Mat4f_Identity()};

    // memory
    {
        application.arena                                     = MemoryArena_Create(Shader_CalculateRequiredBytes(&application.renderer));
        application.components[application.component_count++] = APPLICATION_MEMORY_ARENA_COMPONENT;
    }

    // buffer memory
    {
        // BufferMemory_Create rounds BufferMemoryCreateInfo fields to appropriate offsets
        BufferMemoryCreateInfo buffer_memory_create_info = {
            .vertex_buffer_capacity  = sizeof(vertices),
            .index_buffer_capacity   = sizeof(indices),
            .uniform_buffer_capacity = sizeof(mvp) * application.renderer.frame_count,
        };
        application.buffer_memory = BufferMemory_Create(&application.renderer, &buffer_memory_create_info);
        if (application.buffer_memory.handle == VK_NULL_HANDLE)
        {
            ROSINA_LOG_ERROR("Failed to create buffer memory");
            Application_Cleanup(&application);
            return application;
        }

        application.components[application.component_count++] = APPLICATION_BUFFER_MEMORY_COMPONENT;
    }

    // shader
    {
        application.vbo = VertexBufferObject_Create(sizeof(vertices), &application.buffer_memory);
        application.ibo = IndexBufferObject_Create(sizeof(indices), &application.buffer_memory);

        const ShaderCreateInfo shader_create_info = {
            .fragment_shader_path = "/home/dlk/Documents/code/learning_vulkan/compiled_shaders/fragment.spv",
            .vertex_shader_path   = "/home/dlk/Documents/code/learning_vulkan/compiled_shaders/vertex.spv",
            .arena                = &application.arena,
            .memory               = &application.buffer_memory,
        };
        application.shader = Shader_Create(&application.renderer, &shader_create_info);
        if (application.shader.component_count == 0)
        {
            ROSINA_LOG_ERROR("Failed to create shader");
            Application_Cleanup(&application);
            return application;
        }
        application.components[application.component_count++] = APPLICATION_SHADER_COMPONENT;
    }

    // TODO: Get rid of this. It's bad code. It just shouldn't be here.
    if (Renderer_InitializeGraphicsPipeline(&application.renderer, &application.shader))
    {
        ROSINA_LOG_ERROR("Failed to initialize graphics pipeline");
        Application_Cleanup(&application);
        return application;
    }

    // populate buffers
    {
        StagingBuffer staging_buffer =
            StagingBuffer_Create(&application.renderer, sizeof(vertices) + sizeof(indices) + (sizeof(mvp) * application.renderer.frame_count));

        // start commands
        {
            VK_ERROR_HANDLE(vkResetCommandBuffer(application.renderer.primary_command_buffers[application.renderer.frame_index], 0), {
                ROSINA_LOG_ERROR("Failed to reset command buffer");
                Application_Cleanup(&application);
                return application;
            });
            const VkCommandBufferBeginInfo begin_info = {.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                                         .pNext            = NULL,
                                                         .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                                                         .pInheritanceInfo = 0};
            VK_ERROR_HANDLE(vkBeginCommandBuffer(application.renderer.primary_command_buffers[application.renderer.frame_index], &begin_info), {
                ROSINA_LOG_ERROR("Failed to begin command buffer");
                Application_Cleanup(&application);
                return application;
            });
        }

        // copy operations
        {
            // vertex buffer
            {
                void* data = NULL;
                VK_ERROR_HANDLE(vkMapMemory(application.renderer.device.handle, staging_buffer.memory, 0, sizeof(vertices), 0, &data), {
                    ROSINA_LOG_ERROR("Failed to map vertex buffer memory");
                    Application_Cleanup(&application);
                    return application;
                });
                memcpy(data, vertices, sizeof(vertices));
                vkUnmapMemory(application.renderer.device.handle, staging_buffer.memory);

                const VkBufferCopy2 regions[]     = {{.sType     = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
                                                      .pNext     = NULL,
                                                      .srcOffset = 0,
                                                      .dstOffset = application.vbo.offset,
                                                      .size      = application.vbo.size}};
                const VkCopyBufferInfo2 copy_info = {.sType       = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
                                                     .pNext       = NULL,
                                                     .srcBuffer   = staging_buffer.handle,
                                                     .dstBuffer   = application.buffer_memory.vertex_buffer,
                                                     .regionCount = sizeof(regions) / sizeof(VkBufferCopy2),
                                                     .pRegions    = regions};
                vkCmdCopyBuffer2(application.renderer.primary_command_buffers[application.renderer.frame_index], &copy_info);
            }

            // index buffer
            {
                void* data = NULL;
                VK_ERROR_HANDLE(vkMapMemory(application.renderer.device.handle, staging_buffer.memory, sizeof(vertices), sizeof(indices), 0, &data), {
                    ROSINA_LOG_ERROR("Failed to map index buffer memory");
                    Application_Cleanup(&application);
                    return application;
                });
                memcpy(data, indices, sizeof(indices));
                vkUnmapMemory(application.renderer.device.handle, staging_buffer.memory);

                const VkBufferCopy2 regions[]     = {{.sType     = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
                                                      .pNext     = NULL,
                                                      .srcOffset = sizeof(vertices),
                                                      .dstOffset = application.ibo.offset,
                                                      .size      = application.ibo.size}};
                const VkCopyBufferInfo2 copy_info = {.sType       = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
                                                     .pNext       = NULL,
                                                     .srcBuffer   = staging_buffer.handle,
                                                     .dstBuffer   = application.buffer_memory.index_buffer,
                                                     .regionCount = sizeof(regions) / sizeof(VkBufferCopy2),
                                                     .pRegions    = regions};
                vkCmdCopyBuffer2(application.renderer.primary_command_buffers[application.renderer.frame_index], &copy_info);
            }

            // uniform buffers
            for (uint32_t i = 0; i < application.renderer.frame_count; i++)
            {
                void* data = NULL;
                VK_ERROR_HANDLE(vkMapMemory(application.renderer.device.handle, staging_buffer.memory, sizeof(vertices) + sizeof(indices) + (sizeof(mvp) * i),
                                            sizeof(mvp), 0, &data),
                                {
                                    ROSINA_LOG_ERROR("Failed to map uniform buffer memory");
                                    Application_Cleanup(&application);
                                    return application;
                                });
                memcpy(data, mvp, sizeof(mvp));
                vkUnmapMemory(application.renderer.device.handle, staging_buffer.memory);

                const VkBufferCopy2 regions[]     = {{.sType     = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
                                                      .pNext     = NULL,
                                                      .srcOffset = sizeof(vertices) + sizeof(indices) + (sizeof(mvp) * i),
                                                      .dstOffset = application.shader.ubos[i].offset,
                                                      .size      = application.shader.ubos[i].size}};
                const VkCopyBufferInfo2 copy_info = {.sType       = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
                                                     .pNext       = NULL,
                                                     .srcBuffer   = staging_buffer.handle,
                                                     .dstBuffer   = application.buffer_memory.uniform_buffer,
                                                     .regionCount = sizeof(regions) / sizeof(VkBufferCopy2),
                                                     .pRegions    = regions};
                vkCmdCopyBuffer2(application.renderer.primary_command_buffers[application.renderer.frame_index], &copy_info);
            }
        }

        // end commands
        {
            VK_ERROR_HANDLE(vkEndCommandBuffer(application.renderer.primary_command_buffers[application.renderer.frame_index]), {
                ROSINA_LOG_ERROR("Failed to end command buffer");
                Application_Cleanup(&application);
                return application;
            });
            VK_ERROR_HANDLE(
                vkWaitForFences(application.renderer.device.handle, 1, &application.renderer.in_flight[application.renderer.frame_index], VK_TRUE, UINT64_MAX),
                {
                    ROSINA_LOG_ERROR("Failed to wait for fences");
                    Application_Cleanup(&application);
                    return application;
                });
            VK_ERROR_HANDLE(vkResetFences(application.renderer.device.handle, 1, &application.renderer.in_flight[application.renderer.frame_index]), {
                ROSINA_LOG_ERROR("Failed to reset fences");
                Application_Cleanup(&application);
                return application;
            });

            const VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            const VkSubmitInfo submit_info           = {.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                                        .waitSemaphoreCount   = 0,
                                                        .pWaitSemaphores      = NULL,
                                                        .pWaitDstStageMask    = wait_stages,
                                                        .commandBufferCount   = 1,
                                                        .pCommandBuffers      = application.renderer.primary_command_buffers + application.renderer.frame_index,
                                                        .signalSemaphoreCount = 0,
                                                        .pSignalSemaphores    = NULL};
            VK_ERROR_HANDLE(
                vkQueueSubmit(application.renderer.main_queue.handle, 1, &submit_info, application.renderer.in_flight[application.renderer.frame_index]), {
                    ROSINA_LOG_ERROR("Failed to submit queue");
                    Application_Cleanup(&application);
                    return application;
                });
        }

        StagingBuffer_Cleanup(&application.renderer, &staging_buffer);
    }

    return application;
}

void Application_Run(Application application[static 1])
{
    while (!glfwWindowShouldClose(application->renderer.window.window))
    {
        glfwPollEvents();

        if (Renderer_StartScene(&application->renderer))  // also binds graphics pipeline
            break;

        Shader_Bind(&application->renderer, &application->shader);
        VertexBufferObject_Bind(&application->renderer, &application->buffer_memory, &application->vbo);
        IndexBufferObject_Bind(&application->renderer, &application->buffer_memory, &application->ibo);

        vkCmdDrawIndexed(application->renderer.primary_command_buffers[application->renderer.frame_index], application->ibo.size / sizeof(uint32_t), 1, 0, 0,
                         0);

        if (Renderer_EndScene(&application->renderer)) break;
    }
}