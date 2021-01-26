//------------------------------------------------------------------------------
//
// File Name:	Image.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        7/24/2020 
//
//------------------------------------------------------------------------------
#pragma once

CUSTOM_VK_DECLARE_NO_CREATE(Image, Image, Device)
public:
	void Create(vk::ImageCreateInfo& imageCreateInfo, 
				VmaAllocationCreateInfo& allocCreateInfo,
				Device& owner);

	void Create(glm::uvec2 size, 
				vk::Format format, 
				vk::ImageTiling tiling, 
				vk::ImageUsageFlags usage,
				vk::ImageLayout dstLayout,
				Device& owner);

	void TransitionLayout(vk::Format format,
						  uint32_t mipLevels,
						  vk::ImageLayout oldLayout,
						  vk::ImageLayout newLayout) const;

protected:

	VmaAllocator allocator = {};
	VmaAllocation allocation = {};
	VmaAllocationInfo allocationInfo = {};
	vk::ImageUsageFlags imageUsage = {};
	VmaMemoryUsage memoryUsage = {};
	vk::DeviceSize size = {};
};





