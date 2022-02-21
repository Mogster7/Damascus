//------------------------------------------------------------------------------
//
// File Name:	FrameBufferAttachment.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		7/25/2020
//
//------------------------------------------------------------------------------
#include "FrameBufferAttachment.h"

namespace dm
{

void FrameBufferAttachment::Create(
    vk::Format format,
    vk::Extent2D extent,
    vk::ImageUsageFlags usage,
    vk::ImageAspectFlags aspectMask,
    vk::ImageLayout destinationLayout,
    Device* owner)
{
    IOwned::CreateOwned(owner);
    image.Create2D({ extent.width, extent.height }, format, 1, vk::ImageTiling::eOptimal,
                   usage, destinationLayout, aspectMask, owner);


    this->format = format;
    vk::ImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.image = image.VkType();           // Image to create view for
    viewCreateInfo.viewType = vk::ImageViewType::e2D;// Type of image (1D, 2D, 3D, Cube, etc)
    viewCreateInfo.format = format;                  // Format of image data
    // Subresources allow the view to view only a part of an image
    viewCreateInfo.subresourceRange.aspectMask = aspectMask;// Which aspect of image to view (e.g. COLOR_BIT for viewing colour)
    viewCreateInfo.subresourceRange.baseMipLevel = 0;       // Start mipmap level to view from
    viewCreateInfo.subresourceRange.levelCount = 1;         // Number of mipmap levels to view
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;     // Start array level to view from
    viewCreateInfo.subresourceRange.layerCount = 1;         // Number of array levels to view

    imageView.Create(viewCreateInfo, owner);
}


void FrameBufferAttachment::Create(
    vk::Format format,
    vk::ImageAspectFlags aspectMask,
    vk::ImageCreateInfo& imageCreateInfo,
    Device* owner)
{
    IOwned::CreateOwned(owner);
    this->format = format;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    image.Create(imageCreateInfo, allocInfo, owner);

    vk::ImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.image = image.VkType();           // Image to create view for
    viewCreateInfo.viewType = vk::ImageViewType::e2D;// Type of image (1D, 2D, 3D, Cube, etc)
    viewCreateInfo.format = format;                  // Format of image data
    // Subresources allow the view to view only a part of an image
    viewCreateInfo.subresourceRange.aspectMask = aspectMask;// Which aspect of image to view (e.g. COLOR_BIT for viewing colour)
    viewCreateInfo.subresourceRange.baseMipLevel = 0;       // Start mipmap level to view from
    viewCreateInfo.subresourceRange.levelCount = 1;         // Number of mipmap levels to view
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;     // Start array level to view from
    viewCreateInfo.subresourceRange.layerCount = 1;         // Number of array levels to view

    imageView.Create(viewCreateInfo, owner);
}

void FrameBufferAttachment::CreateSampled(vk::Format format, vk::ImageAspectFlags aspectMask, vk::ImageCreateInfo& imageCreateInfo, vk::SamplerCreateInfo& samplerCreateInfo, Device* owner)
{
    Create(format, aspectMask, imageCreateInfo, owner);
    sampler.Create(samplerCreateInfo, owner);
}

void FrameBufferAttachment::CreateSampled(
    vk::Format format,
    vk::Extent2D extent,
    vk::ImageUsageFlags usage,
    vk::ImageAspectFlags aspectMask,
    vk::ImageLayout destinationLayout,
    vk::SamplerCreateInfo& samplerCreateInfo,
    Device* owner)
{
    Create(format, extent, usage, aspectMask, destinationLayout, owner);
    sampler.Create(samplerCreateInfo, owner);
}



void FrameBufferAttachment::CreateDepth(Device* owner)
{
    format = GetDepthFormat(&owner->OwnerGet<Renderer>());
    Create(format, owner->OwnerGet<Renderer>().swapchain->extent,
           vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc,
           vk::ImageAspectFlagBits::eDepth,
           vk::ImageLayout::eDepthStencilAttachmentOptimal,
           owner);
}

vk::DescriptorImageInfo FrameBufferAttachment::GetDescriptor(vk::ImageLayout imageLayout) const
{
    return vk::DescriptorImageInfo(
        sampler.VkType(),
        imageView.VkType(),
        imageLayout);
}


vk::Format FrameBufferAttachment::GetDepthFormat(Renderer* renderer)
{
    vk::FormatProperties formatProperties{ renderer->physicalDevice.getFormatProperties(vk::Format::eD32Sfloat) };

    if (formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
        return vk::Format::eD32Sfloat;
    }

    assert(false);
    return vk::Format::eUndefined;
}

void FrameBufferAttachment::Destroy()
{
    image.Destroy();
    imageView.Destroy();
    sampler.Destroy();
}

FrameBufferAttachment::~FrameBufferAttachment() noexcept
{
    if (created)
    {
        Destroy();
    }
}



}// namespace dm