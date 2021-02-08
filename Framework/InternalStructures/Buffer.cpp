//------------------------------------------------------------------------------
//
// File Name:	Buffer.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/9/2020
//
//------------------------------------------------------------------------------
#include "RenderingContext.h"
#include "Device.h"

void Buffer::Create(vk::BufferCreateInfo& bufferCreateInfo,
        VmaAllocationCreateInfo& allocCreateInfo, Device& owner)
{
    m_owner = &owner;
    this->allocator = m_owner->allocator;
    size = bufferCreateInfo.size;

    utils::CheckVkResult((vk::Result)vmaCreateBuffer(owner.allocator, (VkBufferCreateInfo*)&bufferCreateInfo, &allocCreateInfo,
        (VkBuffer*)&m_object, &allocation, &allocationInfo), 
        "Failed to allocate buffer");
}

void Buffer::MapToBuffer(void* data)
{
	// Copy view & projection data
	const auto& vpAllocInfo = GetAllocationInfo();
	vk::DeviceMemory memory = vpAllocInfo.deviceMemory;
	vk::DeviceSize offset = vpAllocInfo.offset;
    void* toMap;
	auto result = m_owner->mapMemory(memory, offset, size, {}, &toMap);
	utils::CheckVkResult(result, "Failed to map uniform buffer memory");
	memcpy(toMap, data, size);
	m_owner->unmapMemory(memory);
}

void Buffer::StageTransfer(Buffer& src, Buffer& dst, Device& device, vk::DeviceSize size)
{
    vk::Queue transferQueue = device.graphicsQueue;
    vk::CommandPool transferCmdPool = device.commandPool;

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
    vmaDestroyBuffer(allocator, (VkBuffer)m_object, allocation);
}

void Buffer::CreateStaged(void* data, const vk::DeviceSize size,
    vk::BufferUsageFlags bufferUsage,
    VmaMemoryUsage memoryUsage, 
    Device& owner)
{
    m_owner = &owner;
    this->allocator = m_owner->allocator;

    vk::BufferCreateInfo bCreateInfo;
    bCreateInfo.usage = vk::BufferUsageFlagBits::eTransferDst | bufferUsage;
    bCreateInfo.size = size;

    VmaAllocationCreateInfo aCreateInfo = {};
    aCreateInfo.usage = memoryUsage;

    // Create THIS buffer, which is the destination buffer
    Buffer::Create(bCreateInfo, aCreateInfo, *m_owner);


    // STAGING BUFFER
    Buffer stagingBuffer;

    // Reuse create info, except this time its the source 
    bCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;

    // we can reuse size
    aCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    stagingBuffer.Create(bCreateInfo, aCreateInfo, *m_owner);

    // Create pointer to memory
    void* mapped; 

    //// Map and copy data to the memory, then unmap
    vmaMapMemory(allocator, stagingBuffer.GetAllocation(), &mapped);
    std::memcpy(mapped, data, (size_t)size);
    vmaUnmapMemory(allocator, stagingBuffer.GetAllocation());

    // Copy staging buffer to GPU-side vertex buffer
    StageTransfer(stagingBuffer, *this, *m_owner, size);

    // Cleanup
    stagingBuffer.Destroy();
}

void IndexBuffer::Create(const std::vector<uint32_t> indices, Device& owner)
{
    CreateStaged((void*)indices.data(), indices.size() * sizeof(uint32_t),
        vk::BufferUsageFlagBits::eIndexBuffer, 
		VMA_MEMORY_USAGE_GPU_ONLY,
        owner);
    indexCount = indices.size();
}
