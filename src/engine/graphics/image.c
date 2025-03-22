#include <engine/graphics/image.h>

#include <assert.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void LoadImageIntoBuffer(const char* const path, void* const buffer, uint64_t size [static 1])
{
    assert(size != NULL);

    int w = 0, h = 0, d = 0;

    if (buffer == NULL)
    {
        if (!stbi_info(path, &w, &h, &d))
        {
            ROSINA_LOG_ERROR("Failed to get image image info!");
            *size = 0;
            return;
        }

        *size = w * h * 4;  // VK_FORMAT_R8G8B8A8_SRGB == 4 bytes
        return;
    }

    unsigned char* b = stbi_load(path, &w, &h, &d, 4);
    if (b == NULL)
    {
        ROSINA_LOG_ERROR("Failed to load image.");
        *size = 0;
        return;
    }

    if (*size != w * h * 4)
    {
        ROSINA_LOG_ERROR("Size mismatch");
        *size = 0;
        stbi_image_free(b);
        return;
    }

    memcpy(buffer, b, *size);
    stbi_image_free(b);
}

void Image_Cleanup(Renderer renderer[static 1], Image image [static 1])
{
    vkDestroySampler(renderer->device.handle, image->sampler, NULL);
    image->sampler = VK_NULL_HANDLE;
    vkDestroyImageView(renderer->device.handle, image->view, NULL);
    image->view = VK_NULL_HANDLE;
    vkDestroyBuffer(renderer->device.handle, image->buffer, NULL);
    image->buffer = VK_NULL_HANDLE;
    vkFreeMemory(renderer->device.handle, image->memory, NULL);
    image->memory = VK_NULL_HANDLE;
    vkDestroyImage(renderer->device.handle, image->handle, NULL);
    image->handle = VK_NULL_HANDLE;
}

Image Image_Create(Renderer renderer[static 1], const ImageCreateInfo create_info [static 1])
{
    Image image = {
        .handle = VK_NULL_HANDLE,
        .memory = VK_NULL_HANDLE,
        .view = VK_NULL_HANDLE,
        .sampler = VK_NULL_HANDLE,
        .format = VK_FORMAT_UNDEFINED,
        .width = 0,
        .height = 0,
    };

    {
        int h;
        int w;
        int d;
        if (!stbi_info(create_info->path, &w, &h, &d))
        {
            ROSINA_LOG_ERROR("Failed to load image!");
            return image;
        }
        image.width = (uint32_t)w;
        image.height = (uint32_t)h;
    }

    // TODO: find a way to determine this?
    image.format = VK_FORMAT_R8G8B8A8_SRGB;

    // create image
    {
        const VkImageCreateInfo image_create_info = {
            . sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R8G8B8A8_SRGB,
            .extent = {.width = image.width, .height = image.height, .depth = 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = NULL,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };

        VK_ERROR_HANDLE(vkCreateImage(renderer->device.handle, &image_create_info, NULL, &image.handle), {
            return image;
        });
    }

    // allocate memory
    {
        VkMemoryRequirements reqs;
        vkGetImageMemoryRequirements(renderer->device.handle, image.handle, &reqs);

        const VkMemoryAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = NULL,
            .allocationSize = reqs.size,
            .memoryTypeIndex = FindMemoryType(renderer->device.physical_device, reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        };
        VK_ERROR_HANDLE(vkAllocateMemory(renderer->device.handle, &alloc_info, NULL, &image.memory), {
            vkDestroyImage(renderer->device.handle, image.handle, NULL);
            image.handle = VK_NULL_HANDLE;
            return image;
        });
    }

    // create buffer
    {
        const  VkBufferCreateInfo buffer_create_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .size = Image_CalculateSize(&image),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = NULL
        };
        VK_ERROR_HANDLE(vkCreateBuffer(renderer->device.handle, &buffer_create_info, NULL, &image.buffer), {
            vkFreeMemory(renderer->device.handle, image.memory, NULL);
            image.memory = VK_NULL_HANDLE;
            vkDestroyImage(renderer->device.handle, image.handle, NULL);
            image.handle = VK_NULL_HANDLE;
            return image;
        });
    }

    // bind image to memory
    {
        VkBindImageMemoryInfo bind_infos [] = {
            {
                .sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
                .pNext = NULL,
                .image = image.handle,
                .memory = image.memory,
                .memoryOffset = 0
            }};
        VK_ERROR_HANDLE(vkBindImageMemory2(renderer->device.handle, sizeof(bind_infos)/ sizeof(VkBindImageMemoryInfo), bind_infos), {
            vkDestroyBuffer(renderer->device.handle, image.buffer, NULL);
            image.buffer = VK_NULL_HANDLE;
            vkFreeMemory(renderer->device.handle, image.memory, NULL);
            image.memory = VK_NULL_HANDLE;
            vkDestroyImage(renderer->device.handle, image.handle, NULL);
            image.handle = VK_NULL_HANDLE;
            return image;
        });
    }

    // bind buffer to memory
    {
        {
            const VkBindBufferMemoryInfo bind_infos [] = {{
                .sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
                .pNext = NULL,
                .buffer = image.buffer,
                .memory = image.memory,
                .memoryOffset = 0
            }};
            VK_ERROR_HANDLE(vkBindBufferMemory2(renderer->device.handle, sizeof(bind_infos) / sizeof(VkBindBufferMemoryInfo), bind_infos), {
                vkDestroyBuffer(renderer->device.handle, image.buffer, NULL);
                image.buffer = VK_NULL_HANDLE;
                vkFreeMemory(renderer->device.handle, image.memory, NULL);
                image.memory = VK_NULL_HANDLE;
                vkDestroyImage(renderer->device.handle, image.handle, NULL);
                image.handle = VK_NULL_HANDLE;
                return image;
            });
        }
    }

    {
        const VkImageViewCreateInfo image_view_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = image.handle,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_R8G8B8A8_SRGB,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_R,
                .g = VK_COMPONENT_SWIZZLE_G,
                .b = VK_COMPONENT_SWIZZLE_B,
                .a = VK_COMPONENT_SWIZZLE_A,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        };

        VK_ERROR_HANDLE(vkCreateImageView(renderer->device.handle, &image_view_create_info, NULL, &image.view), {
            vkDestroyBuffer(renderer->device.handle, image.buffer, NULL);
            image.buffer = VK_NULL_HANDLE;
            vkFreeMemory(renderer->device.handle, image.memory, NULL);
            image.memory = VK_NULL_HANDLE;
            vkDestroyImage(renderer->device.handle, image.handle, NULL);
            image.handle = VK_NULL_HANDLE;
        });
    }

    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(renderer->device.physical_device, &properties);

        const VkSamplerCreateInfo sampler_create_info = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
        };
        VK_ERROR_HANDLE(vkCreateSampler(renderer->device.handle, &sampler_create_info, NULL, &image.sampler), {
            vkDestroyImageView(renderer->device.handle, image.view, NULL);
            image.view = VK_NULL_HANDLE;
            vkDestroyBuffer(renderer->device.handle, image.buffer, NULL);
            image.buffer = VK_NULL_HANDLE;
            vkFreeMemory(renderer->device.handle, image.memory, NULL);
            image.memory = VK_NULL_HANDLE;
            vkDestroyImage(renderer->device.handle, image.handle, NULL);
            image.handle = VK_NULL_HANDLE;
        });
    }

    return image;
}

void Image_TransitionLayout(Renderer renderer[static 1], const Image image [static 1], const VkFormat format, const VkImageLayout old_layout, const VkImageLayout new_layout)
{
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = 0,
        .dstAccessMask = 0,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image->handle,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        ROSINA_LOG_ERROR("Could not transition image layout. Layout not supported.");
        return;
    }

    vkCmdPipelineBarrier(
        renderer->primary_command_buffers[renderer->frame_index],
        sourceStage, destinationStage,
        0,
        0, NULL,
        0, NULL  ,
        1, &barrier
    );
}