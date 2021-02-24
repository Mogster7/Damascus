//------------------------------------------------------------------------------
//
// File Name:	Buffer.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/9/2020
//
//------------------------------------------------------------------------------
#include "RenderingContext.h"
#include "Device.h"
#include "Buffer.h"

void Buffer::Create(vk::BufferCreateInfo& bufferCreateInfo,
        VmaAllocationCreateInfo& allocCreateInfo, Device& owner)
{
    m_owner = &owner;
    this->allocator = m_owner->allocator;
    size = bufferCreateInfo.size;
    assert(size != 0);

    utils::CheckVkResult((vk::Result)vmaCreateBuffer(owner.allocator, (VkBufferCreateInfo*)&bufferCreateInfo, &allocCreateInfo,
        (VkBuffer*)&m_object, &allocation, &allocationInfo), 
        "Failed to allocate buffer");

    memoryUsage = allocCreateInfo.usage;
    bufferUsage = bufferCreateInfo.usage;
	descriptorInfo.offset = 0;
    descriptorInfo.range = size;
    descriptorInfo.buffer = Get();
}

void Buffer::Map(const Buffer& buffer, void* data) 
{
	// Copy view & projection data
	vk::DeviceMemory memory = buffer.allocationInfo.deviceMemory;
	vk::DeviceSize offset = buffer.allocationInfo.offset;
    void* toMap;

    // If staging buffer exists, it is persistently mapped
    if (buffer.persistentMapped)
        toMap = buffer.stagingBuffer->allocationInfo.pMappedData;
    else
    {
		auto result = buffer.m_owner->mapMemory(memory, offset, buffer.size, {}, &toMap);
		utils::CheckVkResult(result, "Failed to map uniform buffer memory");
    }

	memcpy(toMap, data, buffer.size);

    if(!buffer.persistentMapped)
		buffer.m_owner->unmapMemory(memory);
}

void Buffer::UpdateData(void* data, vk::DeviceSize size, bool submitToGPU) 
{
    if (this->size < size)
    {
        if (persistentMapped)
            CreateStagedPersistent(
                data, size,
                bufferUsage, memoryUsage,
                submitToGPU,
                *m_owner
            );
        else
            CreateStaged(
                data, size,
                bufferUsage, memoryUsage,
                submitToGPU,
                *m_owner
            );
    }
    else
    {
		memcpy(stagingBuffer->allocationInfo.pMappedData, data, (size_t)size);
        if (submitToGPU)
            StageTransferSingleSubmit(*stagingBuffer, *this, *m_owner, size);
    }
}


void Buffer::MapToBuffer(void* data) const
{
    Map(*this, data);
}

void Buffer::MapToStagingBuffer(void* data) const
{
    Map(*stagingBuffer, data);
}

void* Buffer::GetMappedData()
{
    assert(persistentMapped);

    return stagingBuffer->allocationInfo.pMappedData;
}


const void* Buffer::GetMappedData() const
{
    assert(persistentMapped);

    return stagingBuffer->allocationInfo.pMappedData;
}

void Buffer::StageTransferSingleSubmit(Buffer& src, Buffer& dst, Device& device, vk::DeviceSize size)
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

void Buffer::StageTransfer(
    Buffer& src, 
    Buffer& dst, 
    Device& device, 
    vk::DeviceSize size,
    vk::CommandBuffer commandBuffer)
{
    // Create copy region for command buffer
    vk::BufferCopy copyRegion;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    // Command to copy src to dst
    commandBuffer.copyBuffer(src, dst, 1, &copyRegion);
}

void Buffer::StageTransferDynamic(vk::CommandBuffer commandBuffer)
{
    StageTransfer(*stagingBuffer, *this, *m_owner, size, commandBuffer);
}


void Buffer::Destroy()
{
    if (stagingBuffer != nullptr)
        stagingBuffer->Destroy();
    vmaDestroyBuffer(allocator, (VkBuffer)m_object, allocation);
}

void Buffer::CreateStaged(void* data, const vk::DeviceSize size,
    vk::BufferUsageFlags bufferUsage,
    VmaMemoryUsage memoryUsage,
    bool submitToGPU,
    Device& owner)
{
    assert(size > 0);
    assert(data != nullptr);
    persistentMapped = false;
    m_owner = &owner;
    this->allocator = m_owner->allocator;

    vk::BufferCreateInfo bCreateInfo;
    bCreateInfo.usage = vk::BufferUsageFlagBits::eTransferDst | bufferUsage;
    bCreateInfo.size = size;

    VmaAllocationCreateInfo aCreateInfo = {};
    aCreateInfo.usage = memoryUsage;

    // Create THIS buffer, which is the destination buffer
    Buffer::Create(bCreateInfo, aCreateInfo, *m_owner);

    // Reuse create info, except this time its the source 
    bCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;

    // we can reuse size
    aCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    stagingBuffer = new Buffer();
    stagingBuffer->Create(bCreateInfo, aCreateInfo, *m_owner);

    // Create pointer to memory
    void* mapped; 

    //// Map and copy data to the memory, then unmap
    vmaMapMemory(allocator, stagingBuffer->allocation, &mapped);
    std::memcpy(mapped, data, (size_t)size);
    vmaUnmapMemory(allocator, stagingBuffer->allocation);

    // Copy staging buffer to GPU-side vertex buffer
    if (submitToGPU)
    {
		StageTransferSingleSubmit(*stagingBuffer, *this, *m_owner, size);
    }
}

void Buffer::CreateStagedPersistent(
    void* data,
    const vk::DeviceSize size,
    vk::BufferUsageFlags bufferUsage,
    VmaMemoryUsage memoryUsage,
    bool submitToGPU,
    Device& owner)
{
    assert(size > 0);
    assert(data != nullptr);
    persistentMapped = true;
    m_owner = &owner;
    this->allocator = m_owner->allocator;

    vk::BufferCreateInfo bufCreateInfo;
    bufCreateInfo.usage = vk::BufferUsageFlagBits::eTransferDst | bufferUsage;
    bufCreateInfo.size = size;

    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = memoryUsage;

    // Create THIS buffer, which is the destination buffer
    Buffer::Create(bufCreateInfo, allocCreateInfo, *m_owner);

    // Reuse create info, except this time its the source 
    bufCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;

    // we can reuse size
    allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    stagingBuffer = new Buffer();
    stagingBuffer->Create(bufCreateInfo, allocCreateInfo, *m_owner);

    memcpy(stagingBuffer->allocationInfo.pMappedData, data, (size_t)size);

    if (submitToGPU)
    {
        StageTransferSingleSubmit(
            *stagingBuffer, *this, owner, size
        );
    }
}


std::vector<vk::DescriptorBufferInfo*> Buffer::AggregateDescriptorInfo(std::vector<Buffer>& buffers)
{
    std::vector<vk::DescriptorBufferInfo*> infos;
    for (auto& buffer : buffers)
        infos.emplace_back(&buffer.descriptorInfo);
    return infos;
}

