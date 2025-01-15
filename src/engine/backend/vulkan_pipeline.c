#include <engine/backend/vulkan_helpers.h>

#include <assert.h>
#include <string.h>

#include <utility/load_file.h>

void DestroyVulkanGraphicsPipeline(
    const VulkanDevice                      device                  [static 1],
    VulkanGraphicsPipeline                  pipeline                [static 1]
) {
    while (pipeline->component_count > 0) {
        switch (pipeline->components[--pipeline->component_count]) {
        case VULKAN_GRAPHICS_PIPELINE_VERTEX_SHADER_LAYOUT_COMPONENT:
            vkDestroyDescriptorSetLayout(device->handle, pipeline->vertex_shader_layout, NULL);
            break;
        case VULKAN_GRAPHICS_PIPELINE_FRAGMENT_SHADER_LAYOUT_COMPONENT:
            vkDestroyDescriptorSetLayout(device->handle, pipeline->fragment_shader_layout, NULL);
            break;
        case VULKAN_GRAPHICS_PIPELINE_LAYOUT_COMPONENT:
            vkDestroyPipelineLayout(device->handle, pipeline->layout, NULL);
            break;
        case VULKAN_GRAPHICS_PIPELINE_HANDLE_COMPONENT:
            vkDestroyPipeline(device->handle, pipeline->handle, NULL);
            break;
        default:
            ROSINA_LOG_ERROR("Invalid vulkan graphics pipeline component value");
            assert(false);
            break;
        }
    }
}

bool CreateVulkanGraphicsPipeline(
    const VulkanDevice                      device                  [static 1],
    const VulkanGraphicsPipelineCreateInfo  create_info             [static 1],
    VulkanGraphicsPipeline                  pipeline                [static 1]
) {
    assert(create_info->fragment_shader_path != NULL);
    assert(create_info->vertex_shader_path != NULL);
    assert(create_info->render_pass != NULL);

    {
        const VkDescriptorSetLayoutBinding bindings [] = {{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = NULL
        }};

        const VkDescriptorSetLayoutCreateInfo vertex_create_layout = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .bindingCount = sizeof(bindings) / sizeof(VkDescriptorSetLayoutBinding),
            .pBindings = bindings
        };
        VK_ERROR_HANDLE(vkCreateDescriptorSetLayout(device->handle, &vertex_create_layout, NULL, &pipeline->vertex_shader_layout), {
            DestroyVulkanGraphicsPipeline(device, pipeline);
            return true;
        });
        pipeline->components[pipeline->component_count++] = VULKAN_GRAPHICS_PIPELINE_VERTEX_SHADER_LAYOUT_COMPONENT;
    }

    {
        const VkDescriptorSetLayoutCreateInfo fragment_create_layout = {
            .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext          = NULL,
            .flags          = 0,
            .bindingCount   = 0,
            .pBindings      = NULL
        };
        VK_ERROR_HANDLE(vkCreateDescriptorSetLayout(device->handle, &fragment_create_layout, NULL, &pipeline->fragment_shader_layout), {
            DestroyVulkanGraphicsPipeline(device, pipeline);
            return true;
        });
        pipeline->components[pipeline->component_count++] = VULKAN_GRAPHICS_PIPELINE_FRAGMENT_SHADER_LAYOUT_COMPONENT;
    }

    {
        const VkDescriptorSetLayout layouts [] = {
            pipeline->vertex_shader_layout,
            pipeline->fragment_shader_layout
        };
        const VkPipelineLayoutCreateInfo layout_create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .setLayoutCount = sizeof(layouts) / sizeof(VkDescriptorSetLayout),
            .pSetLayouts = layouts,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = NULL
        };
        VK_ERROR_HANDLE(vkCreatePipelineLayout(device->handle, &layout_create_info, NULL, &pipeline->layout), {
            DestroyVulkanGraphicsPipeline(device, pipeline);
            return true;
        });
        pipeline->components[pipeline->component_count++] = VULKAN_GRAPHICS_PIPELINE_LAYOUT_COMPONENT;
    }

    {
        VkPipelineShaderStageCreateInfo shader_stages [] = {{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = VK_NULL_HANDLE,
            .pName = "main",
            .pSpecializationInfo = NULL,
        }, {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = VK_NULL_HANDLE,
            .pName = "main",
            .pSpecializationInfo = NULL,
        }};
        {
            VkShaderModuleCreateInfo vertex_module_create_info = {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .codeSize = 0,
                .pCode = NULL
            };
            VkShaderModuleCreateInfo fragment_module_create_info = {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .codeSize = 0,
                .pCode = NULL
            };
            void* bytes = NULL;
            {
                if (LoadFile(NULL, &vertex_module_create_info.codeSize, create_info->vertex_shader_path)) {
                    DestroyVulkanGraphicsPipeline(device, pipeline);
                    ROSINA_LOG_ERROR("Could not load file 1");
                    return true;
                }
                if (LoadFile(NULL, &fragment_module_create_info.codeSize, create_info->fragment_shader_path)) {
                    DestroyVulkanGraphicsPipeline(device, pipeline);
                    ROSINA_LOG_ERROR("Could not load file 2");
                    return true;
                }
                const size_t byte_count = (vertex_module_create_info.codeSize > fragment_module_create_info.codeSize) 
                    ? vertex_module_create_info.codeSize 
                    : fragment_module_create_info.codeSize;
                bytes = malloc(byte_count);
                vertex_module_create_info.pCode = bytes;
                fragment_module_create_info.pCode = bytes;
            }

            if (LoadFile(bytes, &vertex_module_create_info.codeSize, create_info->vertex_shader_path)) {
                free(bytes);
                DestroyVulkanGraphicsPipeline(device, pipeline);
                return true;
            }
            VK_ERROR_HANDLE(vkCreateShaderModule(device->handle, &vertex_module_create_info, NULL, &shader_stages[0].module), {
                free(bytes);
                DestroyVulkanGraphicsPipeline(device, pipeline);
                return true;
            });

            memset(bytes, 0, vertex_module_create_info.codeSize);

            if (LoadFile(bytes, &fragment_module_create_info.codeSize, create_info->fragment_shader_path)) {
                free(bytes);
                ROSINA_LOG_ERROR("Could not load file 3");
                DestroyVulkanGraphicsPipeline(device, pipeline);
                return true;
            }
            VK_ERROR_HANDLE(vkCreateShaderModule(device->handle, &fragment_module_create_info, NULL, &shader_stages[1].module), {
                vkDestroyShaderModule(device->handle, shader_stages[0].module, NULL);
                free(bytes);
                DestroyVulkanGraphicsPipeline(device, pipeline);
                ROSINA_LOG_ERROR("Could not load file 4");
                return true;
            });
            
            free(bytes);
        }

        const VkVertexInputAttributeDescription vertex_attrib_descriptions [] = {
            { .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0 }, 
            // { .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, color)      },
            // { .location = 2, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, normal)     },
            // { .location = 3, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT,    .offset = offsetof(Vertex, uv)         }
        };
        const VkVertexInputBindingDescription vertex_binding_descriptions [] = {{
            .binding = 0,
            .stride = (sizeof(float) * 3),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        }};
        const VkPipelineVertexInputStateCreateInfo vertex_input_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = sizeof(vertex_binding_descriptions) / sizeof(VkVertexInputBindingDescription),
            .pVertexBindingDescriptions = vertex_binding_descriptions,
            .vertexAttributeDescriptionCount = sizeof(vertex_attrib_descriptions) / sizeof(VkVertexInputAttributeDescription),
            .pVertexAttributeDescriptions = vertex_attrib_descriptions
        };

        const VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE
        };

        const VkViewport viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = create_info->width,
            .height = create_info->height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };
        const VkRect2D scissor = {
            .offset.x = 0.0f,
            .offset.y = 0.0f,
            .extent = {
                .width = create_info->width,
                .height = create_info->height
            }
        };
        const VkPipelineViewportStateCreateInfo viewport_info = {
            .sType          = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext          = NULL,
            .flags          = 0,
            .viewportCount  = 1,
            .pViewports     = &viewport,
            .scissorCount   = 1,
            .pScissors      = &scissor
        };

        const VkPipelineRasterizationStateCreateInfo rasterization_info = {
            .sType                      = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext                      = NULL,
            .flags                      = 0,
            .depthClampEnable           = VK_FALSE,
            .rasterizerDiscardEnable    = VK_FALSE,
            .polygonMode                = VK_POLYGON_MODE_FILL,
            .cullMode                   = VK_CULL_MODE_NONE,
            .frontFace                  = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable            = VK_FALSE,
            .depthBiasConstantFactor    = 0.0f,
            .depthBiasClamp             = 0.0f,
            .depthBiasSlopeFactor       = 0.0f,
            .lineWidth                  = 1.0f
        };

        const VkPipelineMultisampleStateCreateInfo multisampling_info = {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext                  = NULL,
            .flags                  = 0,
            .rasterizationSamples   = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable    = VK_TRUE,
            .alphaToCoverageEnable  = VK_FALSE,
            .alphaToOneEnable       = VK_FALSE
        };

        const VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable        = VK_TRUE,
            .depthWriteEnable       = VK_TRUE,
            .depthCompareOp         = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable  = VK_FALSE,
            .stencilTestEnable      = VK_FALSE,
            .front                  = {},
            .back                   = {},
            .minDepthBounds         = 0.0f,
            .maxDepthBounds         = 1.0f
        };

        const VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
            .blendEnable            = VK_FALSE,
            .srcColorBlendFactor    = VK_BLEND_FACTOR_ZERO,
            .dstColorBlendFactor    = VK_BLEND_FACTOR_ZERO,
            .colorBlendOp           = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor    = VK_BLEND_FACTOR_ZERO,
            .dstAlphaBlendFactor    = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp           = VK_BLEND_OP_ADD,
            .colorWriteMask         = 
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
        };
        const VkPipelineColorBlendStateCreateInfo color_blend_info = {
            .sType              = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable      = VK_FALSE,
            .logicOp            = VK_LOGIC_OP_COPY,
            .attachmentCount    = 1,
            .pAttachments       = &color_blend_attachment_state,
            .blendConstants     = { 0.0f, 0.0f, 0.0f, 0.0f }
        };

        const VkDynamicState dynamic_states [] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        const VkPipelineDynamicStateCreateInfo dynamic_state = {
            .sType              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext              = NULL,
            .flags              = 0,
            .dynamicStateCount  = sizeof(dynamic_states) / sizeof(VkDynamicState),
            .pDynamicStates     = dynamic_states
        };

        const VkGraphicsPipelineCreateInfo pipeline_create_info = {
            .sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext                  = NULL,
            .flags                  = 0,
            .stageCount             = sizeof(shader_stages) / sizeof(VkPipelineShaderStageCreateInfo),
            .pStages                = shader_stages,
            .pVertexInputState      = &vertex_input_info,
            .pInputAssemblyState    = &input_assembly_info,
            .pTessellationState     = NULL,
            .pViewportState         = &viewport_info,
            .pRasterizationState    = &rasterization_info,
            .pMultisampleState      = &multisampling_info,
            .pDepthStencilState     = &depth_stencil_info,
            .pColorBlendState       = &color_blend_info,
            .pDynamicState          = &dynamic_state,
            .layout                 = pipeline->layout,
            .renderPass             = create_info->render_pass->handle,
            .subpass                = 0,
            .basePipelineHandle     = VK_NULL_HANDLE,
            .basePipelineIndex      = 0
        };

        const VkResult result = vkCreateGraphicsPipelines(device->handle, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &pipeline->handle);

        vkDestroyShaderModule(device->handle, shader_stages[0].module, NULL);
        vkDestroyShaderModule(device->handle, shader_stages[1].module, NULL);

        if (result != VK_SUCCESS) {
            ROSINA_LOG_ERROR("%s", string_VkResult(result));
            DestroyVulkanGraphicsPipeline(device, pipeline);
            return true;
        }

        pipeline->components[pipeline->component_count++] = VULKAN_GRAPHICS_PIPELINE_HANDLE_COMPONENT;
    }

    return false;
}