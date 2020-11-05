//------------------------------------------------------------------------------
//
// File Name:	Buffer.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/9/2020
//
//------------------------------------------------------------------------------
#include "Renderer.h"
#include "Device.h"

void Buffer::Create(vk::BufferCreateInfo& bufferCreateInfo,
        VmaAllocationCreateInfo& allocCreateInfo, Device& owner, VmaAllocator allocator)
{
    m_Owner = &owner;
    m_Allocator = allocator;
    m_Size = bufferCreateInfo.size;

    utils::CheckVkResult((vk::Result)vmaCreateBuffer(m_Allocator, (VkBufferCreateInfo*)&bufferCreateInfo, &allocCreateInfo,
        (VkBuffer*)&m_Buffer, &m_Allocation, &m_AllocationInfo), 
        "Failed to allocate buffer");
}

void Buffer::StageTransfer(Buffer& src, Buffer& dst, Device& device, vk::DeviceSize size)
{
    vk::Queue transferQueue = device.GetGraphicsQueue();
    vk::CommandPool transferCmdPool = device.GetGraphicsCmdPool();

    // Command buffer to hold transfer commands
    vk::CommandBuffer transferCmdBuffer;

    // Command Buffer details
    vk::CommandBufferAllocateInfo allocInfo = {};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = transferCmdPool;
    allocInfo.commandBufferCount = 1;

    // Allocate command buffer from pool
    device.allocateCommandBuffers(&allocInfo, &transferCmdBuffer);

    // Create begin info for command buffer recording
    vk::CommandBufferBeginInfo beginInfo;
    // We're only using the command buffer once
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    // Begin recording transfer commands
    transferCmdBuffer.begin(&beginInfo);

    // Create copy region for command buffer
    vk::BufferCopy copyRegion;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    // Command to copy src to dst
    transferCmdBuffer.copyBuffer(src, dst, 1, &copyRegion);
    
    transferCmdBuffer.end();

    // Queue submission info
    vk::SubmitInfo submitInfo;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferCmdBuffer;
    
    // Submit transfer command to transfer queue and wait until done (not optimal)
    transferQueue.submit(1, &submitInfo, {});
    transferQueue.waitIdle();

    // Free temporary command buffer back to pool
    device.freeCommandBuffers(transferCmdPool, transferCmdBuffer);
}

void Buffer::Destroy()
{
    vmaDestroyBuffer(m_Allocator, (VkBuffer)m_Buffer, m_Allocation);
}

void Buffer::CreateStaged(void* data, uint32_t numElements, uint32_t sizeOfElement, 
    vk::BufferUsageFlags usage, Device& owner, VmaAllocator allocator)
{
    m_Owner = &owner;
    m_Allocator = allocator;
    m_Size = sizeOfElement * numElements;

    vk::BufferCreateInfo bCreateInfo;
    bCreateInfo.usage = usage | vk::BufferUsageFlagBits::eTransferDst;
    bCreateInfo.size = m_Size;

    VmaAllocationCreateInfo aCreateInfo = {};
    aCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    Buffer::Create(bCreateInfo, aCreateInfo, *m_Owner, m_Allocator);


    // STAGING BUFFER
    Buffer stagingBuffer;

    bCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    // we can reuse size

    aCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    stagingBuffer.Create(bCreateInfo, aCreateInfo, *m_Owner, allocator);

    // Create pointer to memory
    void* mapped; 

    //// Map and copy data to the memory, then unmap
    vmaMapMemory(allocator, stagingBuffer.GetAllocation(), &mapped);
    std::memcpy(mapped, data, (size_t)m_Size);
    vmaUnmapMemory(allocator, stagingBuffer.GetAllocation());

    // Copy staging buffer to GPU-side vertex buffer
    StageTransfer(stagingBuffer, *this, *m_Owner, m_Size);

    // Cleanup
    stagingBuffer.Destroy();
}

void VertexBuffer::Create(const eastl::vector<Vertex> vertices, Device& owner, VmaAllocator allocator)
{
    CreateStaged((void*)&vertices[0], vertices.size(), sizeof(Vertex), 
        vk::BufferUsageFlagBits::eVertexBuffer, owner, allocator);
    m_VertexCount = vertices.size();
}

void IndexBuffer::Create(const eastl::vector<uint32_t> indices, Device& owner, VmaAllocator allocator)
{
    CreateStaged((void*)indices.data(), indices.size(), sizeof(uint32_t), 
        vk::BufferUsageFlagBits::eIndexBuffer, owner, allocator);
    m_IndexCount = indices.size();
}
