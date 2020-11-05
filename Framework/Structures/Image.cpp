//------------------------------------------------------------------------------
//
// File Name:	Image.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        7/24/2020 
//
//------------------------------------------------------------------------------
void Image::Create(vk::ImageCreateInfo& imageCreateInfo, VmaAllocationCreateInfo& allocCreateInfo,
    Device& owner, VmaAllocator allocator)
{
    m_Owner = &owner;
    m_Allocator = allocator;

    utils::CheckVkResult((vk::Result)vmaCreateImage(m_Allocator, (VkImageCreateInfo*)&imageCreateInfo, &allocCreateInfo,
        (VkImage*)&m_Image, &m_Allocation, &m_AllocationInfo), 
        "Failed to allocate image");
}


void Image::Create(glm::uvec2 size, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage,
    Device& owner, VmaAllocator allocator)
{
    vk::ImageCreateInfo imageCreateInfo = {};

    imageCreateInfo.imageType = vk::ImageType::e2D;
    imageCreateInfo.extent.width = size.x;
    imageCreateInfo.extent.height = size.y;
    // Just 1, no 3D aspect
    imageCreateInfo.extent.depth = 1; 
    // Number of mipmap levels
    imageCreateInfo.mipLevels = 1;
    // Number of levels in image array (used in cube maps)
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = format;
    // How image data should be arranged for optimal reading
    imageCreateInfo.tiling = tiling;
    // Layout of image data on creation
    imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    // Bit flags defining what image will be used for
    imageCreateInfo.usage = usage;
    // Number of samples for multisampling
    imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
    // Whether image can be shared between queues
    imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    Create(imageCreateInfo, allocInfo, owner, owner.GetMemoryAllocator());
}


void Image::Destroy()
{
    vmaDestroyImage(m_Allocator, m_Image, m_Allocation);
}
