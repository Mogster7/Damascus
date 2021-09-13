//------------------------------------------------------------------------------
//
// File Name:	DepthBuffer.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        7/25/2020 
//
//------------------------------------------------------------------------------
#pragma once
namespace dm {
struct Renderer;

//DM_TYPE(FrameBufferAttachment)
class FrameBufferAttachment : public IOwned<Device>
{
public:
	DM_TYPE_OWNED_BODY(FrameBufferAttachment, IOwned<Device>)

	void Create(
		vk::Format format,
		vk::Extent2D extent,
		vk::ImageUsageFlags usage,
		vk::ImageAspectFlags aspectMask,
		vk::ImageLayout destinationLayout,
		Device* owner
	);

	void Create(
		vk::Format format,
		vk::Extent2D extent,
		vk::ImageUsageFlags usage,
		vk::ImageAspectFlags aspectMask,
		vk::ImageLayout destinationLayout,
		vk::SamplerCreateInfo samplerCreateInfo,
		Device* owner
	);

	void CreateDepth(Device* owner);

	~FrameBufferAttachment() noexcept override = default;

	[[nodiscard]] vk::DescriptorImageInfo GetDescriptor(
		vk::ImageLayout imageLayout
	) const;

	Image image;
	ImageView imageView;
	Sampler sampler;
	vk::Format format = {};

	static vk::Format GetDepthFormat(Renderer* context);
};

}