//------------------------------------------------------------------------------
//
// File Name:	DepthBuffer.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        7/25/2020 
//
//------------------------------------------------------------------------------
#pragma once
namespace dm
{
struct Renderer;

class FrameBufferAttachment : public IOwned<Device>
{
public:
	DM_TYPE_OWNED_BODY(FrameBufferAttachment, IOwned<Device>)

	void Create(
		vk::Format format,
		vk::ImageAspectFlags aspectMask,
        vk::ImageCreateInfo& imageCreateInfo,
		Device* owner
	);

    void Create(
        vk::Format format,
        vk::Extent2D extent,
        vk::ImageUsageFlags usage,
        vk::ImageAspectFlags aspectMask,
        vk::ImageLayout destinationLayout,
        Device* owner);

	void CreateSampled(
		vk::Format format,
		vk::Extent2D extent,
		vk::ImageUsageFlags usage,
		vk::ImageAspectFlags aspectMask,
		vk::ImageLayout destinationLayout,
		vk::SamplerCreateInfo& samplerCreateInfo,
		Device* owner
	);

    void CreateSampled(
        vk::Format format,
        vk::ImageAspectFlags aspectMask,
        vk::ImageCreateInfo& imageCreateInfo,
        vk::SamplerCreateInfo& samplerCreateInfo,
        Device* owner
    );

    void Destroy();

	void CreateDepth(Device* owner);

	~FrameBufferAttachment() noexcept override;

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