#include <sandbox/application.h>

#include <assert.h>
#include <string.h>

static inline bool CreateVulkanGraphicsPipeline(const VulkanDevice device[static 1], const VulkanGraphicsPipelineCreateInfo create_info[static 1],
                                  VulkanGraphicsPipeline pipeline[static 1])
{
    assert(create_info->render_pass != NULL);

    // layout
    {
        const VkDescriptorSetLayout layouts[]               = {create_info->shader_layout};
        const VkPipelineLayoutCreateInfo layout_create_info = {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext                  = NULL,
            .flags                  = 0,
            .setLayoutCount         = sizeof(layouts) / sizeof(VkDescriptorSetLayout),
            .pSetLayouts            = layouts,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges    = NULL,
        };
        VK_ERROR_HANDLE(vkCreatePipelineLayout(device->handle, &layout_create_info, NULL, &pipeline->layout), {
            return true;
        });
    }

    // pipeline
    {
        VkPipelineShaderStageCreateInfo shader_stages[] = {{
            .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage               = VK_SHADER_STAGE_VERTEX_BIT,
            .module              = create_info->vertex_shader_module,
            .pName               = "main",
            .pSpecializationInfo = NULL
        }, {
            .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage               = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module              = create_info->fragment_shader_module,
            .pName               = "main",
            .pSpecializationInfo = NULL,
        }};
        // Shader_GetVkVertexInputAttributeDescription
        const VkVertexInputAttributeDescription vertex_attrib_descriptions[] = {
            {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0},
            { .location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = sizeof(float) * 3 },
        };
        // Shader_GetVkVertexInputAttributeDescription
        const VkVertexInputBindingDescription vertex_binding_descriptions[] = {
            {.binding = 0, .stride = (sizeof(float) * 5), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX}
        };
        const VkPipelineVertexInputStateCreateInfo vertex_input_info = {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = sizeof(vertex_binding_descriptions) / sizeof(VkVertexInputBindingDescription),
            .pVertexBindingDescriptions      = vertex_binding_descriptions,
            .vertexAttributeDescriptionCount = sizeof(vertex_attrib_descriptions) / sizeof(VkVertexInputAttributeDescription),
            .pVertexAttributeDescriptions    = vertex_attrib_descriptions
        };

        const VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
            .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE
        };

        const VkViewport viewport = {.x = 0.0f, .y = 0.0f, .width = create_info->width, .height = create_info->height, .minDepth = 0.0f, .maxDepth = 1.0f};
        const VkRect2D scissor    = {.offset = {.x = 0, .y = 0}, .extent = {.width = create_info->width, .height = create_info->height}};
        const VkPipelineViewportStateCreateInfo viewport_info = {
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext         = NULL,
            .flags         = 0,
            .viewportCount = 1,
            .pViewports    = &viewport,
            .scissorCount  = 1,
            .pScissors     = &scissor,
        };

        const VkPipelineRasterizationStateCreateInfo rasterization_info = {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext                   = NULL,
            .flags                   = 0,
            .depthClampEnable        = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode             = VK_POLYGON_MODE_FILL,
            .cullMode                = VK_CULL_MODE_NONE,
            .frontFace               = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable         = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp          = 0.0f,
            .depthBiasSlopeFactor    = 0.0f,
            .lineWidth               = 1.0f
        };

        const VkPipelineMultisampleStateCreateInfo multisampling_info = {
            .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext                 = NULL,
            .flags                 = 0,
            .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable   = VK_TRUE,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable      = VK_FALSE
        };

        const VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {
            .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable       = VK_TRUE,
            .depthWriteEnable      = VK_TRUE,
            .depthCompareOp        = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable     = VK_FALSE,
            .front                 = {},
            .back                  = {},
            .minDepthBounds        = 0.0f,
            .maxDepthBounds        = 1.0f
        };

        const VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
            .blendEnable         = VK_FALSE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
            .colorBlendOp        = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp        = VK_BLEND_OP_ADD,
            .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
        };
        const VkPipelineColorBlendStateCreateInfo color_blend_info = {
            .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable   = VK_FALSE,
            .logicOp         = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments    = &color_blend_attachment_state,
            .blendConstants  = {0.0f, 0.0f, 0.0f, 0.0f}
        };

        const VkDynamicState dynamic_states[]                = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        const VkPipelineDynamicStateCreateInfo dynamic_state = {
            .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext             = NULL,
            .flags             = 0,
            .dynamicStateCount = sizeof(dynamic_states) / sizeof(VkDynamicState),
            .pDynamicStates    = dynamic_states
        };

        const VkGraphicsPipelineCreateInfo pipeline_create_info = {
            .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext               = NULL,
            .flags               = 0,
            .stageCount          = sizeof(shader_stages) / sizeof(VkPipelineShaderStageCreateInfo),
            .pStages             = shader_stages,
            .pVertexInputState   = &vertex_input_info,
            .pInputAssemblyState = &input_assembly_info,
            .pTessellationState  = NULL,
            .pViewportState      = &viewport_info,
            .pRasterizationState = &rasterization_info,
            .pMultisampleState   = &multisampling_info,
            .pDepthStencilState  = &depth_stencil_info,
            .pColorBlendState    = &color_blend_info,
            .pDynamicState       = &dynamic_state,
            .layout              = pipeline->layout,
            .renderPass          = create_info->render_pass->handle,
            .subpass             = 0,
            .basePipelineHandle  = VK_NULL_HANDLE,
            .basePipelineIndex   = 0
        };

        VK_ERROR_HANDLE(vkCreateGraphicsPipelines(device->handle, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &pipeline->handle), {
            vkDestroyPipelineLayout(device->handle, pipeline->layout, NULL);
            pipeline->layout = VK_NULL_HANDLE;
            return true;
        });
    }

    return false;
}

static inline bool Renderer_InitializeGraphicsPipeline(Renderer renderer[static 1], Shader shader[static 1])
{
    const VulkanGraphicsPipelineCreateInfo pipeline_create_info = {
        .render_pass            = &renderer->render_pass,
        .vertex_shader_module   = shader->vertex_module,
        .fragment_shader_module = shader->fragment_module,
        .shader_layout          = shader->layout,
        .width                  = renderer->window.width,
        .height                 = renderer->window.height
    };
    if (CreateVulkanGraphicsPipeline(&renderer->device, &pipeline_create_info, &renderer->graphics_pipeline))
    {
        return true;
    }

    renderer->components[renderer->component_count++] = RENDERER_GRAPHICS_PIPELINE_COMPONENT;
    return false;
}


void Application_Cleanup(Application application[static 1])
{
    while (application->component_count > 0)
    {
        switch (application->components[--application->component_count])
        {
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
            case APPLICATION_IMAGE_COMPONENT:
                Image_Cleanup(&application->renderer, &application->image);
                break;
            default:
                ROSINA_LOG_ERROR("Invalid application component!");
                assert(false);
        }
    }
}

void HandleKeyboardKeyEvent(const Event e) { printf("Event{%d, %d}\n", (int)e.keyboard_key, (int)e.type); }

void HandleMouseButtonEvent(const Event e) { printf("Event{%d, %d}\n", (int)e.keyboard_key, (int)e.type); }

Application Application_Create()
{
    Application application = {.component_count = 0, .components = {}};

    application.renderer = Renderer_Create();
    if (application.renderer.component_count == 0)
    {
        ROSINA_LOG_ERROR("Failed to create renderer");
        Application_Cleanup(&application);
        return application;
    }
    application.components[application.component_count++] = APPLICATION_RENDERER_COMPONENT;

    const float x = 0.8f;
    const float vertices[] = {
        -x, -x, 0.1f, 1.0f, 0.0f,
         x, -x, 0.1f, 0.0f, 0.0f,
         x,  x, 0.1f, 0.0f, 1.0f,
        -x, x, 0.1f, 1.0f, 1.0f
    };
    const uint32_t indices[] = {0, 1, 2, 3, 2, 0};
    Mat4f mvp[3]             = {Mat4f_Identity(), Mat4f_Identity(), Mat4f_Identity()};

    // memory
    {
        application.arena = MemoryArena_Create(1024);

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

    // image
    {
        const ImageCreateInfo image_create_info = {
            .path = "/home/dlk/Pictures/vk_tutorial_texture.jpg",
        };
        application.image = Image_Create(&application.renderer, &image_create_info);
        if (application.image.handle == VK_NULL_HANDLE)
        {
            ROSINA_LOG_ERROR("Failed to create image");
            Application_Cleanup(&application);
            return application;
        }
        application.components[application.component_count++] = APPLICATION_IMAGE_COMPONENT;
    }

    // shader
    {
        application.vbo = VertexBufferObject_Create(sizeof(vertices), &application.buffer_memory);
        application.ibo = IndexBufferObject_Create(sizeof(indices), &application.buffer_memory);

        const ShaderCreateInfo shader_create_info = {
            .fragment_shader_path = "/home/dlk/CLionProjects/learning_vulkan/compiled_shaders/fragment.spv",
            .vertex_shader_path   = "/home/dlk/CLionProjects/learning_vulkan/compiled_shaders/vertex.spv",
            .arena                = &application.arena,
            .memory               = &application.buffer_memory,
            .image                = &application.image,
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
            StagingBuffer_Create(&application.renderer,
                sizeof(vertices) +
                sizeof(indices) +
                Image_CalculateSize(&application.image) +
                (sizeof(mvp) * (uint64_t)application.renderer.frame_count));

        // start commands
        {
            VK_ERROR_HANDLE(vkResetCommandBuffer(application.renderer.primary_command_buffers[application.renderer.frame_index], 0), {
                ROSINA_LOG_ERROR("Failed to reset command buffer");
                Application_Cleanup(&application);
                return application;
            });
            const VkCommandBufferBeginInfo begin_info = {
                .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext            = NULL,
                .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                .pInheritanceInfo = NULL
            };
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

                const VkBufferCopy2 regions[]     = {{
                    .sType     = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
                    .pNext     = NULL,
                    .srcOffset = 0,
                    .dstOffset = application.vbo.offset,
                    .size      = application.vbo.size
                }};
                const VkCopyBufferInfo2 copy_info = {
                    .sType       = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
                    .pNext       = NULL,
                    .srcBuffer   = staging_buffer.handle,
                    .dstBuffer   = application.buffer_memory.vertex_buffer,
                    .regionCount = sizeof(regions) / sizeof(VkBufferCopy2),
                    .pRegions    = regions
                };
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

                const VkBufferCopy2 regions[]     = {{
                    .sType     = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
                    .pNext     = NULL,
                    .srcOffset = sizeof(vertices),
                    .dstOffset = application.ibo.offset,
                    .size      = application.ibo.size
                }};
                const VkCopyBufferInfo2 copy_info = {
                    .sType       = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
                    .pNext       = NULL,
                    .srcBuffer   = staging_buffer.handle,
                    .dstBuffer   = application.buffer_memory.index_buffer,
                    .regionCount = sizeof(regions) / sizeof(VkBufferCopy2),
                    .pRegions    = regions
                };
                vkCmdCopyBuffer2(application.renderer.primary_command_buffers[application.renderer.frame_index], &copy_info);
            }

            // image
            {
                void* data = NULL;
                const uint64_t offset = sizeof(vertices) + sizeof(indices);
                uint64_t image_size = 0;
                LoadImageIntoBuffer("/home/dlk/Pictures/vk_tutorial_texture.jpg", NULL, &image_size);

                VK_ERROR_HANDLE(vkMapMemory(application.renderer.device.handle, staging_buffer.memory, offset, image_size, 0, &data), {
                    ROSINA_LOG_ERROR("Failed to image memory");
                    Application_Cleanup(&application);
                    return application;
                });
                LoadImageIntoBuffer("/home/dlk/Pictures/vk_tutorial_texture.jpg", data, &image_size);
                if (image_size == 0)
                {
                    ROSINA_LOG_ERROR("Failed to load image into staging buffer");
                    Application_Cleanup(&application);
                    return application;
                }
                vkUnmapMemory(application.renderer.device.handle, staging_buffer.memory);

                Image_TransitionLayout(&application.renderer, &application.image, application.image.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

                const VkBufferImageCopy2 regions[]     = {{
                    .sType     = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
                    .pNext     = NULL,
                    .bufferOffset = sizeof(vertices) + sizeof(indices),
                    .bufferRowLength = 0,
                    .bufferImageHeight = 0,
                    .imageSubresource = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = 0,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                    .imageOffset = { 0, 0, 0 },
                    .imageExtent = { application.image.width, application.image.height, 1 }
                }};
                const VkCopyBufferToImageInfo2 copy_info = {
                    .sType       = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
                    .pNext       = NULL,
                    .srcBuffer   = staging_buffer.handle,
                    .dstImage   = application.image.handle,
                    .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .regionCount = sizeof(regions) / sizeof(VkBufferImageCopy2),
                    .pRegions    = regions
                };

                vkCmdCopyBufferToImage2(application.renderer.primary_command_buffers[application.renderer.frame_index], &copy_info);

                Image_TransitionLayout(&application.renderer, &application.image, application.image.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            // uniform buffers
            {
                void* data = NULL;
                const uint64_t offset = sizeof(vertices) + sizeof(indices) + Image_CalculateSize(&application.image);
                VK_ERROR_HANDLE(vkMapMemory(application.renderer.device.handle, staging_buffer.memory, offset, sizeof(mvp), 0, &data), {
                    ROSINA_LOG_ERROR("Failed to map uniform buffer memory");
                    Application_Cleanup(&application);
                    return application;
                });
                memcpy(data, mvp, sizeof(mvp));
                vkUnmapMemory(application.renderer.device.handle, staging_buffer.memory);

                const VkBufferCopy2 regions[]     = {{
                    .sType     = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
                    .pNext     = NULL,
                    .srcOffset = offset,
                    .dstOffset = application.shader.ubo.offset,
                    .size      = application.shader.ubo.size
                }};
                const VkCopyBufferInfo2 copy_info = {
                    .sType       = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
                    .pNext       = NULL,
                    .srcBuffer   = staging_buffer.handle,
                    .dstBuffer   = application.buffer_memory.uniform_buffer,
                    .regionCount = sizeof(regions) / sizeof(VkBufferCopy2),
                    .pRegions    = regions
                };
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
            const VkSubmitInfo submit_info           = {
                .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .waitSemaphoreCount   = 0,
                .pWaitSemaphores      = NULL,
                .pWaitDstStageMask    = wait_stages,
                .commandBufferCount   = 1,
                .pCommandBuffers      = application.renderer.primary_command_buffers + application.renderer.frame_index,
                .signalSemaphoreCount = 0,
                .pSignalSemaphores    = NULL
            };
            VK_ERROR_HANDLE(vkQueueSubmit(application.renderer.device.graphics_queue.handle, 1, &submit_info, application.renderer.in_flight[application.renderer.frame_index]), {
                Application_Cleanup(&application);
                return application;
            });
        }

        // wait for copy operations to complete
        vkWaitForFences(application.renderer.device.handle, 1, &application.renderer.in_flight[application.renderer.frame_index], VK_TRUE, UINT64_MAX);

        StagingBuffer_Cleanup(&application.renderer, &staging_buffer);
    }

    Window_SetKeyboardEventCallbackFunction(&application.renderer.window, HandleKeyboardKeyEvent);
    Window_SetMouseEventCallbackFunction(&application.renderer.window, HandleMouseButtonEvent);

    return application;
}

void Application_Run(Application application[static 1])
{
    while (!Window_ShouldClose(&application->renderer.window))
    {
        Window_PollEvents(&application->renderer.window);

        if (Renderer_StartScene(&application->renderer)) break;  // also binds graphics pipeline

        Shader_Bind(&application->renderer, &application->shader);
        VertexBufferObject_Bind(&application->renderer, &application->buffer_memory, &application->vbo);
        IndexBufferObject_Bind(&application->renderer, &application->buffer_memory, &application->ibo);

        vkCmdDrawIndexed(application->renderer.primary_command_buffers[application->renderer.frame_index], 6, 1, 0, 0, 0);

        if (Renderer_EndScene(&application->renderer)) break;
    }
}