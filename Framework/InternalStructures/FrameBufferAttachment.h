//------------------------------------------------------------------------------
//
// File Name:	DepthBuffer.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        7/25/2020 
//
//------------------------------------------------------------------------------
#pragma once
class Device;

class FrameBufferAttachment 
{
	Device* m_owner = nullptr;


public:
	void Create(vk::Format format,
									   vk::Extent2D extent,
									   vk::ImageUsageFlags usage,
									   vk::ImageAspectFlags aspectMask,
									   vk::ImageLayout destinationLayout,
									   Device& owner);

	void Create(
		vk::Format format,
		vk::Extent2D extent,
		vk::ImageUsageFlags usage,
		vk::ImageAspectFlags aspectMask,
		vk::ImageLayout destinationLayout,
		vk::SamplerCreateInfo samplerCreateInfo,
		Device& owner
	);
    void CreateDepth(Device& owner);
    void Destroy();

	vk::DescriptorImageInfo GetDescriptor(
		vk::ImageLayout imageLayout
	);

	Image image = {};
    ImageView imageView = {};
    vk::Format format = {};
	vk::Sampler sampler = {};

    static vk::Format GetDepthFormat();
};

