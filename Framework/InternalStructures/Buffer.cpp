//------------------------------------------------------------------------------
//
// File Name:	Buffer.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/9/2020
//
//------------------------------------------------------------------------------
#include "Buffer.h"

namespace bk {


void Buffer::Create(vk::BufferCreateInfo& bufferCreateInfo, VmaAllocationCreateInfo& allocCreateInfo, Device* inOwner)
{
	IOwned::Create(inOwner, [this]()
	{
		vmaDestroyBuffer(allocator, VkType(), allocation);
	});

	allocator = owner->allocator;
	size = bufferCreateInfo.size;
	assert(size != 0);
	bufferUsage = bufferCreateInfo.usage;
	memoryUsage = allocCreateInfo.usage;
	ASSERT_VK(vmaCreateBuffer(owner->allocator,
		(VkBufferCreateInfo * ) & bufferCreateInfo,
		&allocCreateInfo,
		VkCTypePtr(),
		&allocation,
		&allocationInfo
	));

	descriptorInfo.offset = 0;
	descriptorInfo.range = size;
	descriptorInfo.buffer = VkType();
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
		auto result = buffer.owner->mapMemory(memory, offset, buffer.size, {}, &toMap);
		utils::CheckVkResult(result, "Failed to map uniform buffer memory");
	}

	memcpy(toMap, data, buffer.size);

	if (!buffer.persistentMapped)
	{
		buffer.owner->unmapMemory(memory);
	}
}

void Buffer::UpdateData(void* data, vk::DeviceSize size, bool submitToGPU)
{
	if (this->size < size)
	{
		CreateStaged(
			data, size,
			bufferUsage, memoryUsage,
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
	assert(persistentMapped);

	return stagingBuffer->allocationInfo.pMappedData;
}


const void* Buffer::GetMappedData() const
{
	assert(persistentMapped);

	return stagingBuffer->allocationInfo.pMappedData;
}

void Buffer::StageTransferSingleSubmit(Buffer& src, Buffer& dst, vk::DeviceSize size, const Device& device)
{
	CommandPool& commandPool = src.OwnerGet<RenderingContext>().commandPool;
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
}

void Buffer::StageTransferDynamic(vk::CommandBuffer commandBuffer)
{
	StageTransfer(*stagingBuffer, *this, size, commandBuffer, *owner);
}


void Buffer::CreateStaged(
	void* data, const vk::DeviceSize size,
	vk::BufferUsageFlags bufferUsage,
	VmaMemoryUsage memoryUsage,
	bool submitToGPU,
	bool persistentMapped,
	Device* inOwner
)
{
	assert(size > 0);
	assert(data != nullptr);
	this->persistentMapped = persistentMapped;

	vk::BufferCreateInfo bufferCreateInfo;
	bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferDst | bufferUsage;
	bufferCreateInfo.size = size;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = memoryUsage;

	// Create THIS buffer, which is the destination buffer
	Create(bufferCreateInfo, allocCreateInfo, inOwner);

	// Reuse create info, except this time its the source
	bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;

	// we can reuse size
	allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	if (persistentMapped)
	{
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}
	stagingBuffer = std::make_shared<Buffer>();
	stagingBuffer->Create(bufferCreateInfo, allocCreateInfo, inOwner);

	if (persistentMapped)
	{
		memcpy(stagingBuffer->allocationInfo.pMappedData, data, (size_t) size);
	}
	else
	{
		// Create pointer to memory
		void* mapped;

		//// Map and copy data to the memory, then unmap
		vmaMapMemory(allocator, stagingBuffer->allocation, &mapped);
		std::memcpy(mapped, data, (size_t) size);
		vmaUnmapMemory(allocator, stagingBuffer->allocation);
	}

	// Copy staging buffer to GPU-side
	if (submitToGPU)
	{
		StageTransferSingleSubmit(*stagingBuffer, *this, size, *owner);
	}
}


std::vector<vk::DescriptorBufferInfo*> Buffer::AggregateDescriptorInfo(std::vector <Buffer>& buffers)
{
	std::vector < vk::DescriptorBufferInfo * > infos;
	for (auto& buffer : buffers)
		infos.emplace_back(&buffer.descriptorInfo);
	return infos;
}


}