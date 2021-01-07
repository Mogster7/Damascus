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
    void Create(vk::BufferCreateInfo& bufferCreateInfo, VmaAllocationCreateInfo& allocCreateInfo,
        Device& owner, VmaAllocator allocator);

    void CreateStaged(void* data, uint32_t numElements, uint32_t sizeOfElement,
        vk::BufferUsageFlags usage, Device& owner, VmaAllocator allocator);

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


class VertexBuffer : public Buffer
{
public:
    void Create(const std::vector<Vertex> vertices, Device& owner, VmaAllocator allocator);

    uint32_t GetVertexCount() const { return vertexCount; }

private:
    uint32_t vertexCount = 0;
};

class IndexBuffer : public Buffer
{
public:
    void Create(const std::vector<uint32_t> indices, Device& owner, VmaAllocator allocator);

    uint32_t GetIndexCount() const { return indexCount; }
private:
    uint32_t indexCount = 0;
};



