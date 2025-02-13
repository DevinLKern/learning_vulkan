#include <assert.h>
#include <engine/graphics/graphics.h>
#include <utility/load_file.h>
#include <utility/log.h>

Shader Shader_Create(Renderer renderer[static 1], const ShaderCreateInfo create_info[static 1])
{
    assert(create_info->vertex_shader_path != NULL);
    assert(create_info->fragment_shader_path != NULL);
    assert(create_info->arena != NULL);
    assert(create_info->memory != NULL);

    Shader shader = {.component_count = 0,
                     .components      = {},
                     .vertex_layout   = VK_NULL_HANDLE,
                     .fragment_layout = VK_NULL_HANDLE,
                     .vertex_module   = VK_NULL_HANDLE,
                     .fragment_module = VK_NULL_HANDLE,
                     .descriptor_pool = VK_NULL_HANDLE,
                     .descriptor_sets = NULL,
                     .ubos            = NULL};

    // layouts
    {
        const VkDescriptorSetLayoutBinding bindings[]                   = {{.binding            = 0,
                                                                            .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                                            .descriptorCount    = 1,
                                                                            .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
                                                                            .pImmutableSamplers = NULL}};
        const VkDescriptorSetLayoutCreateInfo vertex_layout_create_info = {.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                                           .pNext        = NULL,
                                                                           .flags        = 0,
                                                                           .bindingCount = sizeof(bindings) / sizeof(VkDescriptorSetLayoutBinding),
                                                                           .pBindings    = bindings};
        VK_ERROR_HANDLE(vkCreateDescriptorSetLayout(renderer->device.handle, &vertex_layout_create_info, NULL, &shader.vertex_layout), {
            Shader_Cleanup(renderer, &shader);
            return shader;
        });

        const VkDescriptorSetLayoutCreateInfo fragment_layout_create_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .pNext = NULL, .flags = 0, .bindingCount = 0, .pBindings = NULL};
        VK_ERROR_HANDLE(vkCreateDescriptorSetLayout(renderer->device.handle, &fragment_layout_create_info, NULL, &shader.fragment_layout), {
            vkDestroyDescriptorSetLayout(renderer->device.handle, shader.vertex_layout, NULL);
            Shader_Cleanup(renderer, &shader);
            return shader;
        });

        shader.components[shader.component_count++] = SHADER_DESCRIPTOR_SET_LAYOUTS_COMPONENT;
    }

    // pool
    {
        const VkDescriptorPoolSize pool_sizes[]           = {{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = renderer->frame_count}};
        const VkDescriptorPoolCreateInfo pool_create_info = {.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                                             .pNext         = NULL,
                                                             .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
                                                             .maxSets       = renderer->frame_count,
                                                             .poolSizeCount = sizeof(pool_sizes) / sizeof(VkDescriptorPoolSize),
                                                             .pPoolSizes    = pool_sizes};
        VK_ERROR_HANDLE(vkCreateDescriptorPool(renderer->device.handle, &pool_create_info, NULL, &shader.descriptor_pool), {
            Shader_Cleanup(renderer, &shader);
            return shader;
        });

        shader.components[shader.component_count++] = SHADER_DESCRIPTOR_POOL_COMPONENT;
    }

    // modules
    {
        VkShaderModuleCreateInfo vertex_module_create_info = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .pNext = NULL, .flags = 0, .codeSize = 0, .pCode = NULL};
        VkShaderModuleCreateInfo fragment_module_create_info = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .pNext = NULL, .flags = 0, .codeSize = 0, .pCode = NULL};

        if (LoadFile(NULL, &vertex_module_create_info.codeSize, create_info->vertex_shader_path))
        {
            Shader_Cleanup(renderer, &shader);
            ROSINA_LOG_ERROR("Could not get size of vertex shader.");
            return shader;
        }
        if (LoadFile(NULL, &fragment_module_create_info.codeSize, create_info->fragment_shader_path))
        {
            Shader_Cleanup(renderer, &shader);
            ROSINA_LOG_ERROR("Could not get size of fragment shader.");
            return shader;
        }

        MemoryArena arena = MemoryArena_Create(vertex_module_create_info.codeSize + fragment_module_create_info.codeSize);

        void* vertex_code   = MemoryArena_Allocate(&arena, vertex_module_create_info.codeSize);
        void* fragment_code = MemoryArena_Allocate(&arena, fragment_module_create_info.codeSize);

        if (LoadFile(vertex_code, &vertex_module_create_info.codeSize, create_info->vertex_shader_path))
        {
            Shader_Cleanup(renderer, &shader);
            MemoryArena_Free(&arena);
            ROSINA_LOG_ERROR("Could not load load vertex shader.");
            return shader;
        }
        if (LoadFile(fragment_code, &fragment_module_create_info.codeSize, create_info->fragment_shader_path))
        {
            Shader_Cleanup(renderer, &shader);
            MemoryArena_Free(&arena);
            ROSINA_LOG_ERROR("Could not load load fragment shader.");
            return shader;
        }

        vertex_module_create_info.pCode   = vertex_code;
        fragment_module_create_info.pCode = fragment_code;

        VK_ERROR_HANDLE(vkCreateShaderModule(renderer->device.handle, &vertex_module_create_info, NULL, &shader.vertex_module), {
            MemoryArena_Free(&arena);
            Shader_Cleanup(renderer, &shader);
            return shader;
        });
        VK_ERROR_HANDLE(vkCreateShaderModule(renderer->device.handle, &fragment_module_create_info, NULL, &shader.fragment_module), {
            vkDestroyShaderModule(renderer->device.handle, shader.vertex_module, NULL);
            shader.vertex_module = VK_NULL_HANDLE;
            MemoryArena_Free(&arena);
            Shader_Cleanup(renderer, &shader);
            return shader;
        });

        MemoryArena_Free(&arena);

        shader.components[shader.component_count++] = SHADER_MODULES_COMPONENT;
    }

    // descriptor sets
    {
        shader.descriptor_sets               = MemoryArena_Allocate(create_info->arena, renderer->frame_count * sizeof(VkDescriptorSet));
        VkDescriptorSetLayout* const layouts = calloc(renderer->frame_count, sizeof(VkDescriptorSetLayout));
        for (uint32_t i = 0; i < renderer->frame_count; i++)
        {
            layouts[i] = shader.vertex_layout;
        }

        const VkDescriptorSetAllocateInfo alloc_info = {.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                                        .pNext              = NULL,
                                                        .descriptorPool     = shader.descriptor_pool,
                                                        .descriptorSetCount = renderer->frame_count,
                                                        .pSetLayouts        = layouts};
        VK_ERROR_HANDLE(vkAllocateDescriptorSets(renderer->device.handle, &alloc_info, shader.descriptor_sets), {
            Shader_Cleanup(renderer, &shader);
            free(layouts);
            return shader;
        });

        free(layouts);
        shader.components[shader.component_count++] = SHADER_DESCRIPTOR_SETS_COMPONENT;
    }

    // uniforms
    {
        shader.ubos = MemoryArena_Allocate(create_info->arena, renderer->frame_count * sizeof(UniformBufferObject));
        for (uint32_t i = 0; i < renderer->frame_count; i++)
        {
            shader.ubos[i] = UniformBufferObject_Create(sizeof(Mat4f) * 3, create_info->memory);

            const VkDescriptorBufferInfo buffer_info = {
                .buffer = create_info->memory->uniform_buffer, .offset = shader.ubos[i].offset, .range = shader.ubos[i].size};
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
            vkUpdateDescriptorSets(renderer->device.handle, sizeof(descriptor_writes) / sizeof(VkWriteDescriptorSet), descriptor_writes, 0, NULL);
        }
    }

    return shader;
}

void Shader_Cleanup(const Renderer renderer[static 1], Shader shader[static 1])
{
    while (shader->component_count > 0)
    {
        switch (shader->components[--shader->component_count])
        {
            case SHADER_DESCRIPTOR_POOL_COMPONENT:
                vkDestroyDescriptorPool(renderer->device.handle, shader->descriptor_pool, NULL);
                break;
            case SHADER_DESCRIPTOR_SET_LAYOUTS_COMPONENT:
                vkDestroyDescriptorSetLayout(renderer->device.handle, shader->vertex_layout, NULL);
                shader->vertex_layout = VK_NULL_HANDLE;
                vkDestroyDescriptorSetLayout(renderer->device.handle, shader->fragment_layout, NULL);
                shader->fragment_layout = VK_NULL_HANDLE;
                break;
            case SHADER_DESCRIPTOR_SETS_COMPONENT:
                vkFreeDescriptorSets(renderer->device.handle, shader->descriptor_pool, renderer->frame_count, shader->descriptor_sets);
                shader->descriptor_sets = NULL;
                break;
            case SHADER_MODULES_COMPONENT:
                vkDestroyShaderModule(renderer->device.handle, shader->vertex_module, NULL);
                shader->vertex_module = VK_NULL_HANDLE;
                vkDestroyShaderModule(renderer->device.handle, shader->fragment_module, NULL);
                shader->fragment_module = VK_NULL_HANDLE;
                break;
            default:
                ROSINA_LOG_ERROR("INVALID SHADER COMPONENT");
                assert(false);
        }
    }
}