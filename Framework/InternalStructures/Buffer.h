//------------------------------------------------------------------------------
//
// File Name:	Buffer.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/9/2020 
//
//------------------------------------------------------------------------------
#pragma once

CUSTOM_VK_DECLARE_NO_CREATE(Buffer, Buffer, Device)
public:
    void Create(vk::BufferCreateInfo& bufferCreateInfo, 
        VmaAllocationCreateInfo& allocCreateInfo,
        Device& owner);

    void CreateStaged(void* data,
        vk::DeviceSize size,
        vk::BufferUsageFlags bufferUsage,
        VmaMemoryUsage memoryUsage,
        Device& owner);

    vk::DeviceMemory GetMemory() const { return allocationInfo.deviceMemory; }
    const VmaAllocation& GetAllocation() const { return allocation; }
    const VmaAllocationInfo& GetAllocationInfo() const { return allocationInfo; }
    VmaAllocator GetAllocator() { return allocator; }

    static void StageTransfer(Buffer& src, Buffer& dst, Device& device, vk::DeviceSize size);

protected:

    VmaAllocator allocator = {};
    VmaAllocation allocation = {};
    VmaAllocationInfo allocationInfo = {};
    vk::BufferUsageFlags bufferUsage = {};
    VmaMemoryUsage memoryUsage = {};
    vk::DeviceSize size = {};
};


template <class VertexType>
class VertexBuffer : public Buffer
{
public:
    void Create(const std::vector<VertexType> vertices, Device& owner)
    {
		CreateStaged((void*)&vertices[0], vertices.size() * sizeof(VertexType),
					 vk::BufferUsageFlagBits::eVertexBuffer,
					 VMA_MEMORY_USAGE_GPU_ONLY,
					 owner);
		vertexCount = vertices.size();
    }

    uint32_t GetVertexCount() const { return vertexCount; }

private:
    uint32_t vertexCount = 0;
};

class IndexBuffer : public Buffer
{
public:
    void Create(const std::vector<uint32_t> indices, Device& owner);

    uint32_t GetIndexCount() const { return indexCount; }
private:
    uint32_t indexCount = 0;
};



