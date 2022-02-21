//------------------------------------------------------------------------------
//
// File Name:	Buffer.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/9/2020
//
//------------------------------------------------------------------------------
#include "Buffer.h"

namespace dm
{

void Buffer::Create(
	const vk::BufferCreateInfo& bufferCreateInfo,
	const VmaAllocationCreateInfo& allocCreateInfo,
	Device* inOwner
)
{
    IOwned<Device>::CreateOwned(inOwner);

	bufferCI = bufferCreateInfo;
	allocationCI = allocCreateInfo;
    if (allocationCI.flags & VMA_ALLOCATION_CREATE_MAPPED_BIT)
        persistentMapped = true;

	DM_ASSERT(bufferCreateInfo.size != 0);
	DM_ASSERT_MSG(vmaCreateBuffer(owner->allocator,
							  (VkBufferCreateInfo*) &bufferCreateInfo,
							  &allocCreateInfo,
							  VkCTypePtr(),
							  &allocation,
							  &allocationInfo
	) == VK_SUCCESS, "Failed to create buffer");

	descriptorInfo.offset = 0;
	descriptorInfo.range = bufferCreateInfo.size;
	descriptorInfo.buffer = VkType();
}


void Buffer::Destroy()
{
    if(created)
    {
        vmaDestroyBuffer(owner->allocator, VkCType(), allocation);
    }
}

Buffer::~Buffer() noexcept
{
	Destroy();
}

void Buffer::CreateStaged(
	void* data,
    const vk::DeviceSize size,
	vk::BufferUsageFlags bufferUsage,
	VmaMemoryUsage memoryUsage,
	bool submitToGPU,
	bool inPersistentMapped,
	Device* inOwner
)
{
	assert(size > 0);
    // If data is nullptr, it must not be submitting to GPU
	assert(data != nullptr || !submitToGPU);

	vk::BufferCreateInfo bufferCreateInfo;
	bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferDst | bufferUsage;
	bufferCreateInfo.size = size;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = memoryUsage;

	// Create THIS buffer, which is the destination buffer
	Create(bufferCreateInfo, allocCreateInfo, inOwner);
	persistentMapped = inPersistentMapped;

	// Reuse create info, except this time its the source
	bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;

	// we can reuse size
	allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	if (persistentMapped)
	{
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}
	stagingBuffer = std::make_unique<Buffer>();
	stagingBuffer->Create(bufferCreateInfo, allocCreateInfo, inOwner);

	if (persistentMapped)
	{
        if (data)
        {
            std::memcpy(stagingBuffer->allocationInfo.pMappedData, data, (size_t) size);
        }
        else
        {
            std::memset(stagingBuffer->allocationInfo.pMappedData, 0, (size_t) size);
        }
	}
	else
	{
		// Create pointer to memory
		void* mapped;

		//// Map and copy data to the memory, then unmap
		vmaMapMemory(owner->allocator, stagingBuffer->allocation, &mapped);
        if (data)
        {
            std::memcpy(mapped, data, (size_t) size);
        }
        else
        {
            std::memset(mapped, 0, (size_t) size);
        }
		vmaUnmapMemory(owner->allocator, stagingBuffer->allocation);
	}

	// Copy staging buffer to GPU-side
	if (submitToGPU)
	{
		StageTransferSingleSubmit(*stagingBuffer, *this, size, *owner);
	}
}



void Buffer::Map(Buffer& buffer, void* data)
{
	// Copy view & projection data
	vk::DeviceMemory memory(buffer.allocationInfo.deviceMemory);
	vk::DeviceSize offset = buffer.allocationInfo.offset;
	void* toMap;

	// If staging buffer exists, it is persistently mapped
	if (buffer.persistentMapped)
	{
		toMap = buffer.stagingBuffer->allocationInfo.pMappedData;
	}
	else
	{
		auto result = buffer.owner->mapMemory(memory, offset, buffer.bufferCI.size, {}, &toMap);
		DM_ASSERT_VK(result);
	}

	memcpy(toMap, data, buffer.bufferCI.size);

	if (!buffer.persistentMapped)
	{
		buffer.owner->unmapMemory(memory);
	}

    buffer.dirty = true;
}




void Buffer::UpdateData(void* data, vk::DeviceSize size, bool submitToGPU)
{
	if (bufferCI.size < size)
	{
		// TODO: Make this more efficient than wipe and recreation
		vmaDestroyBuffer(owner->allocator, VkCType(), allocation);
		CreateStaged(
			data, size,
			bufferCI.usage, allocationCI.usage,
			submitToGPU,
			persistentMapped,
			owner
		);
	}
	else
	{
		memcpy(stagingBuffer->allocationInfo.pMappedData, data, (size_t) size);
		if (submitToGPU)
		{
			StageTransferSingleSubmit(*stagingBuffer, *this, size, *owner);
		}
	}
}


void Buffer::MapToBuffer(void* data)
{
	Map(*this, data);
}

void Buffer::MapToStagingBuffer(void* data)
{
	Map(*stagingBuffer, data);
}

void* Buffer::GetMappedData()
{
	DM_ASSERT(persistentMapped);

	return stagingBuffer->allocationInfo.pMappedData;
}


const void* Buffer::GetMappedData() const
{
	DM_ASSERT(persistentMapped);

	return stagingBuffer->allocationInfo.pMappedData;
}

void Buffer::StageTransferSingleSubmit(Buffer& src, Buffer& dst, vk::DeviceSize size, const Device& device)
{
	CommandPool& commandPool = src.OwnerGet<Renderer>().commandPool;
	auto cmdBuf = commandPool.BeginCommandBuffer();

	StageTransfer(src, dst, size, cmdBuf.get(), device);

	commandPool.EndCommandBuffer(cmdBuf.get());
}

void Buffer::StageTransfer(
	Buffer& src,
	Buffer& dst,
	vk::DeviceSize size,
	vk::CommandBuffer commandBuffer,
	const Device& device
)
{
	// Create copy region for command buffer
	vk::BufferCopy copyRegion;
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;

	// Command to copy src to dst
	commandBuffer.copyBuffer(src.VkType(), dst.VkType(), 1, &copyRegion);
    dst.dirty = false;
}

void Buffer::StageTransferDynamic(vk::CommandBuffer commandBuffer)
{
	StageTransfer(*stagingBuffer, *this, bufferCI.size, commandBuffer, *owner);
}

void Buffer::StageTransferDynamicSingleSubmit()
{
    StageTransferSingleSubmit(*stagingBuffer, *this, bufferCI.size, *owner);
}


std::vector<vk::DescriptorBufferInfo*> Buffer::AggregateDescriptorInfo(std::vector<Buffer>& buffers)
{
	std::vector<vk::DescriptorBufferInfo*> infos;
	for (auto& buffer : buffers)
		infos.emplace_back(&buffer.descriptorInfo);
	return infos;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
	assert(this != &other);

	allocation = other.allocation;
	other.allocation = {};

	persistentMapped = other.persistentMapped;
	allocationInfo = other.allocationInfo;
	descriptorInfo = other.descriptorInfo;
	stagingBuffer = std::move(other.stagingBuffer);
	allocationCI = other.allocationCI;
	bufferCI = other.bufferCI;

    VulkanInterface::operator=((VulkanInterface&)other);
    OwnerInterface::operator=(std::move((OwnerInterface&)other));

	return *this;
}

Buffer::Buffer(Buffer&& other) noexcept
{
	*this = std::move(other);
}


}
