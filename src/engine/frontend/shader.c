#include <assert.h>
#include <engine/frontend/graphics.h>
#include <utility/load_file.h>
#include <utility/log.h>

Shader Shader_Create(Renderer renderer[static 1], const ShaderCreateInfo create_info[static 1])
{
    Shader shader = {.component_count = 0,
                     .components      = {},
                     .descriptor_pool = VK_NULL_HANDLE,
                     .vertex_layout   = VK_NULL_HANDLE,
                     .fragment_layout = VK_NULL_HANDLE,
                     .descriptor_sets = NULL,
                     .uniform_buffers = NULL};

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

        VK_ERROR_HANDLE(vkCreateShaderModule(renderer->device.handle, &vertex_module_create_info, NULL, &shader.modules[0]), {
            MemoryArena_Free(&arena);
            Shader_Cleanup(renderer, &shader);
            return shader;
        });
        VK_ERROR_HANDLE(vkCreateShaderModule(renderer->device.handle, &fragment_module_create_info, NULL, &shader.modules[1]), {
            vkDestroyShaderModule(renderer->device.handle, shader.modules[0], NULL);
            MemoryArena_Free(&arena);
            Shader_Cleanup(renderer, &shader);
            return shader;
        });

        MemoryArena_Free(&arena);

        shader.components[shader.component_count++] = SHADER_MODULES_COMPONENT;
    }

    return shader;
}

uint64_t Shader_CalculateRequiredBytes(const Renderer renderer[static 1], const Shader shader[static 1])
{
    return renderer->frame_count * (sizeof(VkDescriptorSet) +  // Shader::descriptor_sets
                                    sizeof(VulkanBuffer)       // Shader::uniform_buffers
                                   );
}

bool Shader_Initialize(Renderer renderer[static 1], Shader shader[static 1], MemoryArena arena[static 1])
{
    shader->descriptor_sets = MemoryArena_Allocate(arena, renderer->frame_count * sizeof(VkDescriptorSet));

    // descriptor sets
    {
        VkDescriptorSetLayout* const layouts = calloc(renderer->frame_count, sizeof(VkDescriptorSetLayout));
        for (uint32_t i = 0; i < renderer->frame_count; i++)
        {
            layouts[i] = shader->vertex_layout;
        }

        const VkDescriptorSetAllocateInfo alloc_info = {.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                                        .pNext              = NULL,
                                                        .descriptorPool     = shader->descriptor_pool,
                                                        .descriptorSetCount = renderer->frame_count,
                                                        .pSetLayouts        = layouts};
        VK_ERROR_HANDLE(vkAllocateDescriptorSets(renderer->device.handle, &alloc_info, shader->descriptor_sets), {
            Shader_Cleanup(renderer, shader);
            free(layouts);
            return true;
        });

        free(layouts);
        shader->components[shader->component_count++] = SHADER_DESCRIPTOR_SETS_COMPONENT;
    }

    // uniform buffers
    {
        shader->uniform_buffers = MemoryArena_Allocate(arena, renderer->frame_count * sizeof(VulkanBuffer));
        for (uint32_t i = 0; i < renderer->frame_count; i++)
        {
            if (CreateVulkanUniformBuffer(&renderer->device, sizeof(Mat4f) * 3, &shader->uniform_buffers[i]))
            {
                Shader_Cleanup(renderer, shader);
                return true;
            }
        }
        shader->components[shader->component_count++] = SHADER_UNIFORM_BUFFERS_COMPONENT;
    }

    return false;
}

void Shader_Bind(const Renderer renderer[static 1], const Shader shader[static 1])
{
    vkCmdBindDescriptorSets(renderer->primary_command_buffers[renderer->frame_index], VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->graphics_pipeline.layout, 0, 1,
                            shader->descriptor_sets + renderer->frame_index, 0, NULL);
}

void Shader_Cleanup(Renderer renderer[static 1], Shader shader[static 1])
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
                vkDestroyDescriptorSetLayout(renderer->device.handle, shader->fragment_layout, NULL);
                break;
            case SHADER_DESCRIPTOR_SETS_COMPONENT:
                vkFreeDescriptorSets(renderer->device.handle, shader->descriptor_pool, renderer->frame_count, shader->descriptor_sets);
                break;
            case SHADER_MODULES_COMPONENT:
                vkDestroyShaderModule(renderer->device.handle, shader->modules[0], NULL);
                vkDestroyShaderModule(renderer->device.handle, shader->modules[1], NULL);
                break;
            case SHADER_UNIFORM_BUFFERS_COMPONENT:
                for (uint32_t i = 0; i < renderer->frame_count; i++)
                {
                    DestroyVulkanBuffer(&renderer->device, shader->uniform_buffers + i);
                }
                break;
            default:
                ROSINA_LOG_ERROR("INVALID SHADER COMPONENT");
                assert(false);
        }
    }
}