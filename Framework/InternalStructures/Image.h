//------------------------------------------------------------------------------
//
// File Name:	Image.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        7/24/2020 
//
//------------------------------------------------------------------------------
#pragma once

namespace dm {

//DM_TYPE(Image)
class Image : public IVulkanType<vk::Image>, public IOwned<Device>
{
public:
	DM_TYPE_VULKAN_OWNED_BODY(Image, IOwned<Device>)

	void Create(
		vk::ImageCreateInfo& imageCreateInfo,
		VmaAllocationCreateInfo& allocCreateInfo,
		Device* owner
	);

	~Image() noexcept;

	void Create2D(
		glm::uvec2 size,
		vk::Format format,
		uint32_t mipLevels,
		vk::ImageTiling tiling,
		vk::ImageUsageFlags usage,
		vk::ImageLayout dstLayout,
		vk::ImageAspectFlags aspectMask,
		Device* owner
	);

	void CreateDepthImage(glm::vec2 size, Device* owner);

	void TransitionLayout(
		vk::ImageLayout oldLayout,
		vk::ImageLayout newLayout,
		vk::ImageAspectFlags aspectMask,
		uint32_t mipLevels = 1
	);

	void TransitionLayout(
		vk::CommandBuffer commandBuffer,
		vk::ImageLayout oldLayout,
		vk::ImageLayout newLayout,
		vk::ImageAspectFlags aspectMask,
		uint32_t mipLevels = 1
	);

protected:

	VmaAllocator allocator = {};
	VmaAllocation allocation = {};
	VmaAllocationInfo allocationInfo = {};
	vk::ImageUsageFlags imageUsage = {};
	VmaMemoryUsage memoryUsage = {};
	vk::DeviceSize size = {};
};


}