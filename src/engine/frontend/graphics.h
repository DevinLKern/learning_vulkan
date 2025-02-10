#ifndef ROSINA_ENGINE_FRONTEND_RENDERER_H
#define ROSINA_ENGINE_FRONTEND_RENDERER_H

#include <engine/backend/glfw_helpers.h>
#include <engine/backend/vulkan_helpers.h>
#include <utility/math.h>
#include <utility/memory_arena.h>

typedef enum RendererComponent
{
    RENDERER_WINDOW_COMPONENT,
    RENDERER_CONTEXT_COMPONENT,
    RENDERER_DEVICE_COMPONENT,
    RENDERER_RENDER_PASS_COMPONENT,
    RENDERER_GRAPHICS_PIPELINE_COMPONENT,
    RENDERER_SWAPCHAIN_COMPONENT,
    RENDERER_MEMORY_COMPONENT,
    RENDERER_SWAPCHAIN_IMAGES_COMPONENT,
    RENDERER_DEPTH_IMAGES_COMPONENT,
    RENDERER_DEPTH_IMAGE_MEMORIES_COMPONENT,
    RENDERER_SWAPCHAIN_IMAGE_VIEWS_COMPONENT,
    RENDERER_DEPTH_IMAGE_VIEWS_COMPONENT,
    RENDERER_FRAMEBUFFERS_COMPONENT,
    RENDERER_IMAGE_AVAILABLE_COMPONENT,
    RENDERER_RENDER_FINISHED_COMPONENT,
    RENDERER_FRAME_IN_FLIGHT_COMPONENT,
    RENDERER_COMMAND_POOLS_COMPONENT,
    RENDERER_PRIMARY_COMMAND_BUFFERS_COMPONENT,
    RENDERER_COMPONENT_CAPACITY
} RendererComponent;

typedef struct Renderer
{
    uint32_t component_count;
    RendererComponent components[RENDERER_COMPONENT_CAPACITY];
    GLFWWindow window;
    VulkanDevice device;
    VulkanRenderPass render_pass;
    VulkanGraphicsPipeline graphics_pipeline;
    VulkanSwapchain swapchain;

    uint32_t image_capacity;
    uint32_t image_count;
    uint32_t frame_capacity;
    uint32_t frame_count;
    MemoryArena memory;
    VkImage* swapchain_images;
    VkImage* depth_images;
    VkDeviceMemory* depth_image_memories;
    VkImageView* depth_image_views;
    VkImageView* swapchain_image_views;
    VkFramebuffer* framebuffers;
    VkSemaphore* image_available;
    VkCommandPool* command_pools;
    VkCommandBuffer* primary_command_buffers;
    VkSemaphore* render_finished;
    VkFence* in_flight;

    VulkanQueue main_queue;
    uint32_t image_index;
    uint32_t frame_index;

    // use in_flight?
    // struct StagingBuffer
    // {
    //     VkDeviceSize capacity;
    //     VkDeviceSize size;
    //     VkBuffer handle;
    //     VkDeviceMemory memory;
    // } staging_buffer;
} Renderer;

Renderer Renderer_Create();

uint64_t Renderer_CalculateRequiredBytes(const Renderer renderer[static 1]);

void Renderer_Cleanup(Renderer renderer[static 1]);

typedef struct StagingBuffer
{
    VkDeviceMemory memory;
    VkBuffer handle;
} StagingBuffer;

StagingBuffer StagingBuffer_Create(const Renderer renderer[static 1], const uint64_t size);

void StagingBuffer_Cleanup(const Renderer renderer[static 1], StagingBuffer buffer[static 1]);

typedef struct BufferObject
{
    VkDeviceSize size;
    VkDeviceSize offset;
} BufferObject;

typedef BufferObject VertexBufferObject;
typedef BufferObject IndexBufferObject;

typedef BufferObject UniformBufferObject;

typedef struct BufferInfo
{
    uint32_t vbo_count;
    VertexBufferObject* vbos;
    uint32_t ibo_count;
    IndexBufferObject* ibos;
    uint32_t uniform_buffer_count;
    UniformBufferObject* uniform_buffers;
} BufferInfo;

typedef enum BufferMemoryComponent
{
    BUFFER_MEMORY_VERTEX_BUFFERS_COMPONENT,
    BUFFER_MEMORY_ibos_COMPONENT,
    BUFFER_MEMORY_UNIFORM_BUFFERS_COMPONENT,
    BUFFER_MEMORY_HANDLE_COMPONENT,
    BUFFER_MEMORY_COMPONENT_COUNT
} BufferMemoryComponent;

typedef struct BufferMemory
{
    uint32_t component_count;
    BufferMemoryComponent components[BUFFER_MEMORY_COMPONENT_COUNT];
    VkBuffer vertex_buffer;
    VkBuffer index_buffer;
    VkBuffer uniform_buffer;
    VkDeviceMemory handle;
} BufferMemory;

void BufferMemory_Cleanup(const Renderer renderer[static 1], BufferMemory memory[static 1]);

/**
 * Each buffer referenced in the BufferInfo struct must have the size field be > 0 and the offset field be UINT64_MAX.
 */
BufferMemory BufferMemory_Create(const Renderer renderer[static 1], const BufferInfo buffers[static 1]);

static inline void VertexBufferObject_Bind(const Renderer renderer[static 1], const BufferMemory memory[static 1], const VertexBufferObject vbo[static 1])
{
    vkCmdBindVertexBuffers(renderer->primary_command_buffers[renderer->frame_index], 0, 1, &memory->vertex_buffer, &vbo->offset);
}

static inline void IndexBufferObject_Bind(const Renderer renderer[static 1], const BufferMemory memory[static 1], const IndexBufferObject ibo[static 1])
{
    vkCmdBindIndexBuffer(renderer->primary_command_buffers[renderer->frame_index], memory->index_buffer, ibo->offset, VK_INDEX_TYPE_UINT32);
}

typedef enum ShaderComponent
{
    SHADER_DESCRIPTOR_POOL_COMPONENT,
    SHADER_DESCRIPTOR_SET_LAYOUTS_COMPONENT,
    SHADER_DESCRIPTOR_SETS_COMPONENT,
    SHADER_MODULES_COMPONENT,
    SHADER_UNIFORM_BUFFERS_COMPONENT,
    SHADER_UNIFORM_BUFFER_MEMORY_COMPONENT,
    SHADER_COMPONENT_CAPACITY
} ShaderComponent;

typedef struct Shader
{
    uint32_t component_count;
    ShaderComponent components[SHADER_COMPONENT_CAPACITY];
    VkDescriptorSetLayout vertex_layout;
    VkDescriptorSetLayout fragment_layout;
    VkShaderModule vertex_module;
    VkShaderModule fragment_module;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet* descriptor_sets;
    UniformBufferObject* ubos;
} Shader;

typedef struct ShaderCreateInfo
{
    const char* vertex_shader_path;
    const char* fragment_shader_path;
    MemoryArena* arena;
    BufferMemory* memory;
} ShaderCreateInfo;

Shader Shader_Create(Renderer renderer[static 1], const ShaderCreateInfo create_info[static 1]);

uint64_t Shader_CalculateRequiredBytes(const Renderer renderer[static 1]);

void Shader_Bind(const Renderer renderer[static 1], const Shader shader[static 1]);

// void Shader_UploadUniformBuffer(const Renderer renderer[static 1], Shader shader[static 1], const VulkanBuffer uniform_buffer);

void Shader_Cleanup(const Renderer renderer[static 1], Shader shader[static 1]);

bool Renderer_InitializeGraphicsPipeline(Renderer renderer[static 1], Shader shader[static 1]);

bool Renderer_StartScene(Renderer renderer[static 1]);

bool Renderer_EndScene(Renderer renderer[static 1]);

#endif