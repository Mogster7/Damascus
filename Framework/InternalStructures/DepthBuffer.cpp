//------------------------------------------------------------------------------
//
// File Name:	DepthBuffer.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		7/25/2020
//
//------------------------------------------------------------------------------
#include "RenderingContext.h"


void DepthBuffer::Create(Device& owner)
{
	m_owner = &owner;
	format = GetDepthFormat();

	image.CreateDepthImage(owner.swapchain.GetExtentDimensions(), owner);

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

	ImageView::Create(imageView, viewCreateInfo, *m_owner);
}

vk::Format DepthBuffer::GetDepthFormat()
{
	vk::FormatProperties formatProperties{ RenderingContext::GetPhysicalDevice().getFormatProperties(vk::Format::eD32Sfloat) };

	if (formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
	{
		return vk::Format::eD32Sfloat;
	}

	ASSERT(false, "32-bit signed depth stencil format not supported");
}

void DepthBuffer::Destroy()
{
	imageView.Destroy();
	image.Destroy();
}

