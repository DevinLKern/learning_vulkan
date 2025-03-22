#include <engine/graphics/shader.h>

#include <assert.h>

#include <utility/load_file.h>
#include <utility/log.h>

Shader Shader_Create(Renderer renderer[static 1], const ShaderCreateInfo create_info[static 1])
{
    assert(create_info->vertex_shader_path != NULL);
    assert(create_info->fragment_shader_path != NULL);
    assert(create_info->arena != NULL);
    assert(create_info->memory != NULL);

    Shader shader = {
        .component_count = 0,
        .components      = {},
        .layout          = VK_NULL_HANDLE,
        .vertex_module   = VK_NULL_HANDLE,
        .fragment_module = VK_NULL_HANDLE,
        .descriptor_pool = VK_NULL_HANDLE,
        .descriptor_set  = VK_NULL_HANDLE,
        .ubo             = {.offset = 0, .size = 0},
    };

    // layouts
    {
        const VkDescriptorSetLayoutBinding bindings[] = {{
            .binding            = 0,
            .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount    = 1,
            .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = NULL
        }, {
            .binding            = 1,
            .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount    = 1,
            .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = NULL
        }};
        const VkDescriptorSetLayoutCreateInfo vertex_layout_create_info = {
            .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext        = NULL,
            .flags        = 0,
            .bindingCount = sizeof(bindings) / sizeof(VkDescriptorSetLayoutBinding),
            .pBindings    = bindings
        };
        VK_ERROR_HANDLE(vkCreateDescriptorSetLayout(renderer->device.handle, &vertex_layout_create_info, NULL, &shader.layout), {
            Shader_Cleanup(renderer, &shader);
            return shader;
        });

        shader.components[shader.component_count++] = SHADER_DESCRIPTOR_SET_LAYOUTS_COMPONENT;
    }

    // pool
    {
        const VkDescriptorPoolSize pool_sizes[] = {
            {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = renderer->frame_count},
            {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = renderer->frame_count}
        };
        uint32_t max_sets = 0;
        for (uint32_t i = 0; i < sizeof(pool_sizes) / sizeof(VkDescriptorPoolSize); i++)
        {
            max_sets += pool_sizes[i].descriptorCount;
        }
        const VkDescriptorPoolCreateInfo pool_create_info = {
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext         = NULL,
            .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets       = max_sets,
            .poolSizeCount = sizeof(pool_sizes) / sizeof(VkDescriptorPoolSize),
            .pPoolSizes    = pool_sizes
        };
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
        VkDescriptorSetLayout layout = shader.layout;

        const VkDescriptorSetAllocateInfo alloc_info = {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext              = NULL,
            .descriptorPool     = shader.descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts        = &shader.layout
        };
        VK_ERROR_HANDLE(vkAllocateDescriptorSets(renderer->device.handle, &alloc_info, &shader.descriptor_set), {
            Shader_Cleanup(renderer, &shader);
            return shader;
        });

        shader.components[shader.component_count++] = SHADER_DESCRIPTOR_SETS_COMPONENT;
    }

    // uniforms
    {
        {
            shader.ubo = UniformBufferObject_Create(sizeof(Mat4f) * 3, create_info->memory);

            const VkDescriptorBufferInfo buffer_info = {
                .buffer = create_info->memory->uniform_buffer, .offset = shader.ubo.offset, .range = shader.ubo.size};
            const VkDescriptorImageInfo image_info = {
                .sampler = create_info->image->sampler, .imageView = create_info->image->view, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };

            const VkWriteDescriptorSet descriptor_writes[] = {{
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext            = NULL,
                .dstSet           = shader.descriptor_set,
                .dstBinding       = 0,
                .dstArrayElement  = 0,
                .descriptorCount  = 1,
                .descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pImageInfo       = NULL,
                .pBufferInfo      = &buffer_info,
                .pTexelBufferView = NULL
            }, {
                .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext            = NULL,
                .dstSet           = shader.descriptor_set,
                .dstBinding       = 1,
                .dstArrayElement  = 0,
                .descriptorCount  = 1,
                .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo       = &image_info, // Need to set image info. May require creating a sampler.
                .pBufferInfo      = NULL,
                .pTexelBufferView = NULL
            }};
            vkUpdateDescriptorSets(renderer->device.handle, sizeof(descriptor_writes) / sizeof(VkWriteDescriptorSet), descriptor_writes, 0, NULL);
        }
    }

    return shader;
}

void Shader_Cleanup(const Renderer renderer[static 1], Shader shader[static 1])
{
    vkDeviceWaitIdle(renderer->device.handle);
    
    while (shader->component_count > 0)
    {
        switch (shader->components[--shader->component_count])
        {
            case SHADER_DESCRIPTOR_POOL_COMPONENT:
                vkDestroyDescriptorPool(renderer->device.handle, shader->descriptor_pool, NULL);
                break;
            case SHADER_DESCRIPTOR_SET_LAYOUTS_COMPONENT:
                vkDestroyDescriptorSetLayout(renderer->device.handle, shader->layout, NULL);
                shader->layout = VK_NULL_HANDLE;
                break;
            case SHADER_DESCRIPTOR_SETS_COMPONENT:
                vkFreeDescriptorSets(renderer->device.handle, shader->descriptor_pool, 1, &shader->descriptor_set);
                shader->descriptor_set = NULL;
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

void Shader_GetVkVertexInputAttributeDescription(Shader shader [static 1], uint32_t n [static 1], VkVertexInputAttributeDescription* const dest)
{
    *n = 1;

    if (dest == NULL)
    {
        return;
    }

    dest[0].location = 0;
    dest[0].binding = 0;
    dest[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    dest[0].offset = 0;
}
