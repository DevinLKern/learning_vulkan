#ifndef IMAGE_H
#define IMAGE_H

#include <engine/graphics/renderer.h>

/**
 *
 * @param path The path of the image.
 * @param buffer The buffer that the image contents will be loaded into. If NULL, only the size will be set.
 * @param size The size of the buffer. On error, this value will be 0.
 * @return void
 */
void LoadImageIntoBuffer(const char* const path, void* const buffer, uint64_t size [static 1]);

typedef struct Image
{
    VkImage handle;
    VkImageView view;
    VkDeviceMemory memory;
    VkBuffer buffer;
    VkSampler sampler;
    VkFormat format;
    uint32_t width;
    uint32_t height;
} Image;

void Image_Cleanup(Renderer renderer[static 1], Image image [static 1]);

static inline uint64_t Image_CalculateSize(const Image image [static 1])
{
    const uint64_t size = (uint64_t)image->width * (uint64_t)image->height;
    switch (image->format)
    {
        case VK_FORMAT_R8G8B8A8_SRGB: return size * 4;
        default:
            ROSINA_LOG_ERROR("Invalid image format");
        return 0;
    }
}

typedef struct ImageCreateInfo
{
    const char* path;
} ImageCreateInfo;

Image Image_Create(Renderer renderer[static 1], const ImageCreateInfo create_info [static 1]);

void Image_TransitionLayout(Renderer renderer[static 1], const Image image [static 1], const VkFormat format, const VkImageLayout old_layout, const VkImageLayout new_layout);

#endif
