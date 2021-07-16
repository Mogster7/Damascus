//------------------------------------------------------------------------------
//
// File Name:	FrameBufferAttachment.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		7/25/2020
//
//------------------------------------------------------------------------------
#include "RenderingContext.h"
#include "FrameBufferAttachment.h"

namespace dm {


void FrameBufferAttachment::Create(
	vk::Format format,
	vk::Extent2D extent,
	vk::ImageUsageFlags usage,
	vk::ImageAspectFlags aspectMask,
	vk::ImageLayout destinationLayout,
	Device* owner
)
{
	image.Create2D({ extent.width, extent.height }, format, 1, vk::ImageTiling::eOptimal,
		usage, destinationLayout, aspectMask, owner);


	this->format = format;
	vk::ImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.image = image.VkType();                                            // Image to create view for
	viewCreateInfo.viewType = vk::ImageViewType::e2D;                        // Type of image (1D, 2D, 3D, Cube, etc)
	viewCreateInfo.format = format;                                            // Format of image data
	// Subresources allow the view to view only a part of an image
	viewCreateInfo.subresourceRange.aspectMask = aspectMask;  // Which aspect of image to view (e.g. COLOR_BIT for viewing colour)
	viewCreateInfo.subresourceRange.baseMipLevel = 0;                        // Start mipmap level to view from
	viewCreateInfo.subresourceRange.levelCount = 1;                            // Number of mipmap levels to view
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;                        // Start array level to view from
	viewCreateInfo.subresourceRange.layerCount = 1;                    // Number of array levels to view

	imageView.Create(viewCreateInfo, owner);
}

void FrameBufferAttachment::Create(
	vk::Format format,
	vk::Extent2D extent,
	vk::ImageUsageFlags usage,
	vk::ImageAspectFlags aspectMask,
	vk::ImageLayout destinationLayout,
	vk::SamplerCreateInfo samplerCreateInfo,
	Device* owner
)
{
	Create(format, extent, usage, aspectMask, destinationLayout, owner);
	sampler.Create(samplerCreateInfo, owner);
}


void FrameBufferAttachment::CreateDepth(Device* owner)
{
	format = GetDepthFormat(&owner->OwnerGet<RenderingContext>());
	Create(format, owner->OwnerGet<RenderingContext>().swapchain.extent,
		vk::ImageUsageFlagBits::eDepthStencilAttachment,
		vk::ImageAspectFlagBits::eDepth,
		vk::ImageLayout::eDepthStencilAttachmentOptimal,
		owner);
}

vk::DescriptorImageInfo FrameBufferAttachment::GetDescriptor(
	vk::ImageLayout imageLayout
) const
{
	return vk::DescriptorImageInfo(
		sampler.VkType(),
		imageView.VkType(),
		imageLayout
	);
}


vk::Format FrameBufferAttachment::GetDepthFormat(RenderingContext* context)
{
	vk::FormatProperties formatProperties{ context->physicalDevice.getFormatProperties(vk::Format::eD32Sfloat) };

	if (formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
	{
		return vk::Format::eD32Sfloat;
	}

	assert(false);
	return vk::Format::eUndefined;
}




}