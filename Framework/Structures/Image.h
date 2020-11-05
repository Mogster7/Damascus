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
    void Create(vk::ImageCreateInfo& imageCreateInfo, VmaAllocationCreateInfo& allocCreateInfo,
        Device& owner, VmaAllocator allocator);

    void Create(glm::uvec2 size, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage,
        Device& owner, VmaAllocator allocator);


protected:

    VmaAllocator m_Allocator = {};
    VmaAllocation m_Allocation = {};
    VmaAllocationInfo m_AllocationInfo = {};
    vk::ImageUsageFlags m_ImageUsage = {};
    VmaMemoryUsage m_MemoryUsage = {};
    vk::DeviceSize m_Size = {};
};





