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
    void Create(vk::BufferCreateInfo &bufferCreateInfo,
                VmaAllocationCreateInfo &allocCreateInfo,
                Device &owner);

    void CreateStaged(void *data,
                      vk::DeviceSize size,
                      vk::BufferUsageFlags bufferUsage,
                      VmaMemoryUsage memoryUsage,
                      bool submitToGPU,
                      Device &owner);

    void CreateStagedPersistent(void *data,
                                vk::DeviceSize size,
                                vk::BufferUsageFlags bufferUsage,
                                VmaMemoryUsage memoryUsage,
                                bool submitToGPU,
                                Device &owner);

    void MapToBuffer(void *data);

    void MapToStagingBuffer(void *data);

    void *GetMappedData();

    const void *GetMappedData() const;

    static void StageTransferSingleSubmit(
            Buffer &src,
            Buffer &dst,
            Device &device,
            vk::DeviceSize size
    );

    static void StageTransfer(
            Buffer &src,
            Buffer &dst,
            Device &device,
            vk::DeviceSize size,
            vk::CommandBuffer commandBuffer
    );

    void UpdateData(void *data, vk::DeviceSize size, bool submitToGPU);

    void StageTransferDynamic(vk::CommandBuffer commandBuffer);

    static std::vector<vk::DescriptorBufferInfo *> AggregateDescriptorInfo(std::vector<Buffer> &buffers);

    VmaAllocator allocator = {};
    VmaAllocation allocation = {};
    bool persistentMapped = false;
    VmaAllocationInfo allocationInfo = {};
    vk::BufferUsageFlags bufferUsage = {};
    VmaMemoryUsage memoryUsage = {};
    vk::DeviceSize size = {};
    vk::DescriptorBufferInfo descriptorInfo = {};
    Buffer *stagingBuffer = {};

    static void Map(Buffer &buffer, void *data);
};


template<class VertexType>
class VertexBuffer : public Buffer
{
public:
    void CreateStatic(const std::vector<VertexType> &vertices, Device &owner)
    {
        ASSERT(vertices.size(), "No vertices contained in mesh creation parameters");
        CreateStagedPersistent(
                (void *) &vertices[0], vertices.size() * sizeof(VertexType),
                vk::BufferUsageFlagBits::eVertexBuffer,
                VMA_MEMORY_USAGE_GPU_ONLY,
                true,
                owner
        );
        vertexCount = vertices.size();
    }

    void CreateDynamic(const std::vector<VertexType> &vertices, Device &owner)
    {
        ASSERT(vertices.size(), "No vertices contained in mesh creation parameters");
        CreateStagedPersistent(
                (void *) &vertices[0], vertices.size() * sizeof(VertexType),
                vk::BufferUsageFlagBits::eVertexBuffer,
                VMA_MEMORY_USAGE_GPU_ONLY,
                false,
                owner
                );
        vertexCount = vertices.size();
    }

    void UpdateData(void *data, vk::DeviceSize size, uint32_t newVertexCount, bool submitToGPU)
    {
        vertexCount = newVertexCount;
        Buffer::UpdateData(data, size, submitToGPU);
    }

    uint32_t GetVertexCount() const
    { return vertexCount; }

private:
    uint32_t vertexCount = 0;
};

class IndexBuffer : public Buffer
{
public:

    void CreateStatic(const std::vector<uint32_t> &indices, Device &owner)
    {
        CreateStagedPersistent((void *) indices.data(), indices.size() * sizeof(uint32_t),
                               vk::BufferUsageFlagBits::eIndexBuffer,
                               VMA_MEMORY_USAGE_GPU_ONLY,
                               true,
                               owner);
        indexCount = indices.size();
    }

    void CreateDynamic(const std::vector<uint32_t> &indices, Device &owner)
    {
        CreateStagedPersistent((void *) indices.data(), indices.size() * sizeof(uint32_t),
                               vk::BufferUsageFlagBits::eIndexBuffer,
                               VMA_MEMORY_USAGE_GPU_ONLY,
                               false,
                               owner);
        indexCount = indices.size();
    }


    void UpdateData(void *data, vk::DeviceSize size, uint32_t newIndexCount, bool submitToGPU)
    {
        indexCount = newIndexCount;
        Buffer::UpdateData(data, size, submitToGPU);
    }


    uint32_t GetIndexCount() const
    { return indexCount; }

private:
    uint32_t indexCount = 0;
};



