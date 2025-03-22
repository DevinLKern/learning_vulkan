#ifndef SHADER_H
#define SHADER_H

#include <engine/graphics/renderer.h>

#include <engine/graphics/buffer.h>
#include <engine/graphics/image.h>

typedef enum ShaderComponent
{
    SHADER_DESCRIPTOR_POOL_COMPONENT,
    SHADER_DESCRIPTOR_SET_LAYOUTS_COMPONENT,
    SHADER_DESCRIPTOR_SETS_COMPONENT,
    SHADER_MODULES_COMPONENT,
    SHADER_COMPONENT_CAPACITY
} ShaderComponent;

typedef struct Shader
{
    uint32_t component_count;
    ShaderComponent components[SHADER_COMPONENT_CAPACITY];
    VkDescriptorSetLayout layout;
    VkShaderModule vertex_module;
    VkShaderModule fragment_module;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;
    UniformBufferObject ubo;
} Shader;

/**
 *
 * @param renderer The renderer that was used to create the shader.
 * @param shader The shader created by the renderer.
 */
void Shader_Cleanup(const Renderer renderer[static 1], Shader shader[static 1]);

typedef struct ShaderCreateInfo
{
    const char* vertex_shader_path;
    const char* fragment_shader_path;
    MemoryArena* arena;
    BufferMemory* memory;
    Image* image;
} ShaderCreateInfo;

/**
 *
 * @param renderer The renderer that will be used to create the shader.
 * @param create_info All necessary information for creating the shader.
 * @return The created image. On error, the component_count field will be 0.
 */
Shader Shader_Create(Renderer renderer[static 1], const ShaderCreateInfo create_info[static 1]);

static inline uint64_t Shader_CalculateRequiredBytes(const Renderer renderer[static 1])
{
    return renderer->frame_count * (sizeof(VkDescriptorSet) +      // Shader::descriptor_sets
                                    sizeof(UniformBufferObject));  // Shader::ubo
}

/**
 *
 * @param shader The shader
 * @param n The number of VkVertexInputAttributeDescription needed. On error, this will be UINT32_MAX.
 * @param dest A pointer to an array of n VkVertexInputAttributeDescriptions. If NULL, only n will be set.
 */
void Shader_GetVkVertexInputAttributeDescription(Shader shader [static 1], uint32_t n [static 1], VkVertexInputAttributeDescription* const dest);

static inline void Shader_Bind(const Renderer renderer[static 1], const Shader shader[static 1])
{
    vkCmdBindDescriptorSets(
        renderer->primary_command_buffers[renderer->frame_index],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        renderer->graphics_pipeline.layout,
        0,
        1,
        &shader->descriptor_set,
        0,
        NULL
    );
}

#endif
