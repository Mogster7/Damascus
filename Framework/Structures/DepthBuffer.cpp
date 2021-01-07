//------------------------------------------------------------------------------
//
// File Name:	DepthBuffer.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		7/25/2020
//
//------------------------------------------------------------------------------
#include "Renderer.h"


void DepthBuffer::Create(Device& owner)
{
    m_owner = &owner;
    format = ChooseFormat(
        { vk::Format::eD32SfloatS8Uint, vk::Format::eD32Sfloat,
          vk::Format::eD24UnormS8Uint },
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );

    image.Create(m_owner->swapchain.GetExtentDimensions(), 
        format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment,
        *m_owner, m_owner->allocator);

	vk::ImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.image = image;											// Image to create view for
	viewCreateInfo.viewType = vk::ImageViewType::e2D;						// Type of image (1D, 2D, 3D, Cube, etc)
	viewCreateInfo.format = format;											// Format of image data
	viewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;			// Allows remapping of rgba components to other rgba values
	viewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
	viewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
	viewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;

	// Subresources allow the view to view only a part of an image
	viewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;  // Which aspect of image to view (e.g. COLOR_BIT for viewing colour)
	viewCreateInfo.subresourceRange.baseMipLevel = 0;						// Start mipmap level to view from
	viewCreateInfo.subresourceRange.levelCount = 1;							// Number of mipmap levels to view
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;						// Start array level to view from
	viewCreateInfo.subresourceRange.layerCount = 1;							// Number of array levels to view

    imageView.Create(imageView, viewCreateInfo, *m_owner);
}

vk::Format DepthBuffer::ChooseFormat(
    const std::vector<vk::Format>& formats, 
    vk::ImageTiling tiling, 
    vk::FormatFeatureFlagBits features
)
{
    const auto& physicalDevice = Renderer::GetPhysicalDevice();
    for (auto format : formats)
    {
        // Get properties for format on this device
        vk::FormatProperties props;
        physicalDevice.getFormatProperties(format, &props);

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    throw std::runtime_error("Failed to find a suitable image format");
}

void DepthBuffer::Destroy()
{
    imageView.Destroy();
    image.Destroy();
}

