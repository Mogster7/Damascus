//------------------------------------------------------------------------------
//
// File Name:	Texture.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        02/03/2020
//
//------------------------------------------------------------------------------
#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

namespace dm
{

void Texture::Create(const std::string& path, Device* inOwner)
{
    IOwned<Device>::CreateOwned(inOwner);

    pixelData =
        stbi_load(
            path.c_str(),
            &width,
            &height,
            &channels,
            STBI_rgb_alpha);

    vk::DeviceSize imageSize = width * height * 4;
    uint32_t mipLevels = {
        static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1
    };
    StageTexture(imageSize, mipLevels);
}

void Texture::StageTexture(vk::DeviceSize size, uint32_t mipLevels)
{
    DM_ASSERT_MSG(pixelData != nullptr, "Failed to load texture image");
    DM_ASSERT_MSG(width != 0 && height != 0, "Attempting to stage texture with 0 dimensions");

    vk::BufferCreateInfo stageInfo = {};
    stageInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    stageInfo.size = size;

    VmaAllocationCreateInfo stageAllocInfo = {};
    stageAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

    Buffer stagingBuffer = {};
    stagingBuffer.Create(stageInfo, stageAllocInfo, owner);

    // TODO: abstract staging buffers
    void* mapped;
    auto allocator = owner->allocator;
    // Map and copy data to the memory, then unmap
    vmaMapMemory(allocator, stagingBuffer.allocation, &mapped);
    std::memcpy(mapped, pixelData, (size_t) size);
    vmaUnmapMemory(allocator, stagingBuffer.allocation);

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    image.Create2D(glm::uvec2(width, height),
                   vk::Format::eR8G8B8A8Unorm,
                   mipLevels,
                   vk::ImageTiling::eOptimal,
                   vk::ImageUsageFlagBits::eTransferDst |
                       vk::ImageUsageFlagBits::eTransferSrc |
                       vk::ImageUsageFlagBits::eSampled,
                   vk::ImageLayout::eTransferDstOptimal,
                   vk::ImageAspectFlagBits::eColor,
                   owner);

    vk::ImageSubresourceLayers imageSubresource{
        vk::ImageAspectFlagBits::eColor, // Aspect mask
        0,                               // Mip level
        0,                               // Base array layer
        1
    }; // Layer count

    auto& commandPool = OwnerGet<Renderer>().commandPool;
    vk::UniqueCommandBuffer cmdBuf = commandPool.BeginCommandBuffer();

    vk::Extent3D imageExtent{
        static_cast<uint32_t>(width),  // Width
        static_cast<uint32_t>(height), // Height
        1
    }; // Depth

    vk::BufferImageCopy bufferImageCopy{
        0,                // Buffer offset
        0,                // Buffer row length
        0,                // Buffer image height
        imageSubresource, // Image subresource
        vk::Offset3D(),   // Image offset
        imageExtent
    }; // Image extent

    vk::BufferImageCopy imageCopy = {};
    imageCopy.imageSubresource = imageSubresource;
    imageCopy.imageOffset = vk::Offset3D();
    imageCopy.imageExtent = imageExtent;

    cmdBuf->copyBufferToImage(stagingBuffer.VkType(),
                              image.VkType(),
                              vk::ImageLayout::eTransferDstOptimal,
                              1,
                              &bufferImageCopy);

    commandPool.EndCommandBuffer(cmdBuf.get());

    // Transition to shader resource
    image.TransitionLayout(vk::ImageLayout::eTransferDstOptimal,
                           vk::ImageLayout::eShaderReadOnlyOptimal,
                           vk::ImageAspectFlagBits::eColor,
                           mipLevels);

    imageView.CreateTexture2DView(image.VkType(), owner);

    //    vk::SamplerCreateInfo samplerInfo = {};
    //    samplerInfo.magFilter = vk::Filter::eLinear;
    //    samplerInfo.minFilter = vk::Filter::eLinear;
    //    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    //    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    //    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    //    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    //    samplerInfo.unnormalizedCoordinates = false;
    //    samplerInfo.compareEnable = false;
    //    samplerInfo.compareOp = vk::CompareOp::eNever;
    //    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    //
    //    sampler.Create(samplerInfo, owner)
}


vk::DescriptorImageInfo Texture::GetDescriptor(vk::ImageLayout imageLayout)
{
    return {
        sampler.VkType(),
        imageView.VkType(),
        imageLayout
    };
}
void Texture::Destroy()
{
    if (created)
    {
        stbi_image_free(pixelData);
    }
}

Texture::~Texture()
{
    Destroy();
}

void FontTexture::Create(const std::string& path, Device* inOwner)
{
    IOwned::CreateOwned(inOwner);

    /* load font file */
    long size;
    unsigned char* fontBuffer;

    FILE* fontFile = fopen(path.c_str(), "rb");
    fseek(fontFile, 0, SEEK_END);
    size = ftell(fontFile); /* how long is the file ? */
    fseek(fontFile, 0, SEEK_SET); /* reset */

    fontBuffer = (unsigned char*)malloc(size);

    fread(fontBuffer, size, 1, fontFile);
    fclose(fontFile);

    /* prepare font */
    info = new stbtt_fontinfo();
    if (!stbtt_InitFont(info, fontBuffer, 0))
    {
        printf("failed\n");
    }

    auto extent = OwnerGet<Renderer>().swapchain->extent;
    width = extent.width; /* bitmap width */
    height = extent.height; /* bitmap height */
    lineHeight = 128; /* line height */

    /* create a bitmap for the phrase */
    unsigned char* bitmap = (unsigned char*)calloc(width * height, sizeof(unsigned char));

    /* calculate font scaling */
    scale = stbtt_ScaleForPixelHeight(info, lineHeight);

    const char* word = " !#$%&'()*+,-./0123456789:;<=>?`ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    int x = 0;
    int y = 0;

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(info, &ascent, &descent, &lineGap);

    ascent = roundf(ascent * scale);
    descent = roundf(descent * scale);

    int wrapCount = 0;
    int padding = 32;
    int advance;
    for (int i = 0; i < strlen(word); ++i)
    {
        /* how wide is this character */
        int ax;
        int lsb;
        stbtt_GetCodepointHMetrics(info, word[i], &ax, &lsb);

        /* get bounding box for character (may be offset to account for chars that dip above or below the line */
        int c_x1, c_y1, c_x2, c_y2;
        stbtt_GetCodepointBitmapBox(info, word[i], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

        advance = roundf(ax * scale) + padding;
        if (x + advance > width)
        {
            x = 0;
            wrapCount++;
        }

        /* compute y (different characters have different heights */
        int y = ascent + c_y1 + lineHeight * wrapCount;

        /* render character (stride and offset is important here) */
        int byteOffset = x + roundf(lsb * scale) + (y * width);
        stbtt_MakeCodepointBitmap(info, bitmap + byteOffset, c_x2 - c_x1, c_y2 - c_y1, width, scale, scale, word[i]);

        // Calculate from top-left (account for flipped y)
        glm::vec2 wh = { (c_x2 - c_x1), glm::abs(c_y2 - c_y1) };

        // to UV
        wh /= glm::vec2(width, height);

        glm::vec2 xy(x
                         + roundf(lsb * scale)
                         , y );
        xy /= glm::vec2(width, height);

        characters[word[i]] = glm::vec4(xy, wh);

        /* advance x */
        x += advance;
    }

    pixelData = malloc(sizeof(unsigned char) * width*height*4);
    std::memset(pixelData, 255, sizeof(unsigned char) * width*height*4);

    auto* pixels = (unsigned char*)pixelData;

    // Turn each pixel from bitmap into a full RGBA pixel
    for(int i = 0; i < width*height; ++i)
    {
        unsigned char pixel = bitmap[i];
        // Get alpha
        pixels[i * 4 + 3] = pixel;
    }

    uint32_t mipLevels = {
        static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1
    };
    StageTexture(width * height * 4, mipLevels);
}

void FontTexture::GetFontPosition(char c, glm::vec2& uvScale, glm::vec2& uvOffset)
{
    glm::vec4 xywh = characters[c];
    int ax;
    int lsb;
    stbtt_GetCodepointHMetrics(info, c, &ax, &lsb);
    uvScale = glm::vec2(xywh[2], xywh[3]);
    uvOffset = (glm::vec2)xywh;
}

float FontTexture::GetKerning(char c1, char c2)
{
    int kern;
    kern = stbtt_GetCodepointKernAdvance(info, c1, c2);
    float horizontalOffset = roundf(kern * scale);
    return horizontalOffset;
}


float FontTexture::BuildWord(const std::string& word, bool normalizeWord,
                             std::vector<float>& letterAdvances,
                             std::vector<float>& letterProportions,
                             std::vector<float>& letterOffsets)
{
    letterProportions.resize(word.length());
    letterOffsets.resize(word.length());
    letterAdvances.resize(word.length());
    std::vector<int> leftBearings(word.length());
    int x = 0;

    // TODO: Support newlines, y descent for lowercase chars, etc
    int y = 0;

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(info, &ascent, &descent, &lineGap);

    ascent = roundf(ascent * scale);
    descent = roundf(descent * scale);

    int advance;
    for (int i = 0; i < word.length(); ++i)
    {
        /* how wide is this character */
        int ax;
        stbtt_GetCodepointHMetrics(info, word[i], &ax, &leftBearings[i]);

        /* get bounding box for character (may be offset to account for chars that dip above or below the line */
        int c_x1, c_y1, c_x2, c_y2;
        stbtt_GetCodepointBitmapBox(info, word[i], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

        // Advance by the width + kerning with the next character
        advance = roundf(ax * scale) + GetKerning(word[i], word[i+1]);

        letterAdvances[i] = (float)(advance) / (float)width;
        letterProportions[i] = (float)(c_x2 - c_x1) / (float)width;

        /* advance x */
        x += advance;
    }

    float wordWidth = normalizeWord ? (float)x / (float)width : 1.0f;

    // Normalize to word width if requested
    for(int i = 0; i < letterProportions.size(); ++i)
    {
        letterAdvances[i] /= wordWidth;
        letterProportions[i] /= wordWidth;
        float bitmapSpaceBearing = (float)leftBearings[i] * scale / (float)width;
        float wordSpaceBearing = bitmapSpaceBearing / wordWidth;
        letterOffsets[i] = wordSpaceBearing;
    }

    return wordWidth;
}

void FontTexture::Destroy()
{
    if (created)
    {
        free(pixelData);
        created = false;
    }
}

FontTexture::~FontTexture() noexcept
{
    Destroy();
}

}