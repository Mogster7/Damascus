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

    vk::DeviceMemory GetMemory() const { return m_AllocationInfo.deviceMemory; }
    const VmaAllocation& GetAllocation() const { return m_Allocation; }
    const VmaAllocationInfo& GetAllocationInfo() const { return m_AllocationInfo; }
    VmaAllocator GetAllocator() { return m_Allocator; }

    static void StageTransfer(Buffer& src, Buffer& dst, Device& device, vk::DeviceSize size);

protected:

    VmaAllocator m_Allocator = {};
    VmaAllocation m_Allocation = {};
    VmaAllocationInfo m_AllocationInfo = {};
    vk::BufferUsageFlags m_BufferUsage = {};
    VmaMemoryUsage m_MemoryUsage = {};
    vk::DeviceSize m_Size = {};
};


class VertexBuffer : public Buffer
{
public:
    void Create(const eastl::vector<Vertex> vertices, Device& owner, VmaAllocator allocator);

    uint32_t GetVertexCount() const { return m_VertexCount; }

private:
    uint32_t m_VertexCount = 0;
};

class IndexBuffer : public Buffer
{
public:
    void Create(const eastl::vector<uint32_t> indices, Device& owner, VmaAllocator allocator);

    uint32_t GetIndexCount() const { return m_IndexCount; }
private:
    uint32_t m_IndexCount = 0;
};



