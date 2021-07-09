//------------------------------------------------------------------------------
//
// File Name:	Buffer.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/9/2020 
//
//------------------------------------------------------------------------------
#pragma once
namespace bk {

class Device;

class Buffer : public IVulkanType<vk::Buffer, VkBuffer>, public IOwned<Device>
{
public:
BK_TYPE_VULKAN_OWNED_BODY(Buffer, IOwned<Device>)

	void Create(
		vk::BufferCreateInfo& bufferCreateInfo,
		VmaAllocationCreateInfo& allocCreateInfo,
		Device* owner
	);

	void CreateStaged(
		void* data,
		vk::DeviceSize size,
		vk::BufferUsageFlags bufferUsage,
		VmaMemoryUsage memoryUsage,
		bool submitToGPU,
		bool persistentMapped,
		Device* owner
	);


	void MapToBuffer(void* data);

	void MapToStagingBuffer(void* data);

	void* GetMappedData();

	[[nodiscard]] const void* GetMappedData() const;

	static void StageTransferSingleSubmit(
		Buffer& src,
		Buffer& dst,
		vk::DeviceSize size,
		const Device& device
	);

	static void StageTransfer(
		Buffer& src,
		Buffer& dst,
		vk::DeviceSize size,
		vk::CommandBuffer commandBuffer,
		const Device& device
	);

	void UpdateData(void* data, vk::DeviceSize size, bool submitToGPU);

	void StageTransferDynamic(vk::CommandBuffer commandBuffer);

	static std::vector<vk::DescriptorBufferInfo*> AggregateDescriptorInfo(std::vector<Buffer>& buffers);

	VmaAllocator allocator = {};
	VmaAllocation allocation = {};
	bool persistentMapped = false;
	VmaAllocationInfo allocationInfo = {};
	vk::BufferUsageFlags bufferUsage = {};
	VmaMemoryUsage memoryUsage = {};
	vk::DeviceSize size = {};
	vk::DescriptorBufferInfo descriptorInfo = {};
	std::shared_ptr<Buffer> stagingBuffer = {};

	static void Map(Buffer& buffer, void* data);

private:
};


template<class VertexType>
class VertexBuffer : public Buffer
{
public:
BK_TYPE_OWNED_BODY(VertexBuffer<VertexType>, Buffer)

	void CreateStatic(const std::vector<VertexType>& vertices, Device* owner)
	{
		ASSERT(vertices.size(), "No vertices contained in mesh creation parameters");
		CreateStaged(
			(void*) &vertices[0], vertices.size() * sizeof(VertexType),
			vk::BufferUsageFlagBits::eVertexBuffer,
			VMA_MEMORY_USAGE_GPU_ONLY,
			true,
			true,
			owner
		);
		vertexCount = vertices.size();
	}

	void CreateDynamic(const std::vector<VertexType>& vertices, Device* owner)
	{
		ASSERT(vertices.size(), "No vertices contained in mesh creation parameters");
		CreateStaged(
			(void*) &vertices[0], vertices.size() * sizeof(VertexType),
			vk::BufferUsageFlagBits::eVertexBuffer,
			VMA_MEMORY_USAGE_GPU_ONLY,
			false,
			true,
			owner
		);
		vertexCount = vertices.size();
	}

	void UpdateData(void* data, vk::DeviceSize size, uint32_t newVertexCount, bool submitToGPU)
	{
		vertexCount = newVertexCount;
		Buffer::UpdateData(data, size, submitToGPU);
	}

	[[nodiscard]] uint32_t GetVertexCount() const
	{
		return vertexCount;
	}

private:
	uint32_t vertexCount = 0;
};


//BK_TYPE(IndexBuffer)
class IndexBuffer : public Buffer
{
public:
	BK_TYPE_OWNED_BODY(IndexBuffer, Buffer)

	void CreateStatic(const std::vector<uint32_t>& indices, Device* owner)
	{
		CreateStaged((void*) indices.data(), indices.size() * sizeof(uint32_t),
			vk::BufferUsageFlagBits::eIndexBuffer,
			VMA_MEMORY_USAGE_GPU_ONLY,
			true,
			true,
			owner
		);
		indexCount = indices.size();
	}

	void CreateDynamic(const std::vector<uint32_t>& indices, Device* owner)
	{
		CreateStaged((void*) indices.data(), indices.size() * sizeof(uint32_t),
			vk::BufferUsageFlagBits::eIndexBuffer,
			VMA_MEMORY_USAGE_GPU_ONLY,
			false,
			true,
			owner
		);
		indexCount = indices.size();
	}


	void UpdateData(void* data, vk::DeviceSize size, uint32_t newIndexCount, bool submitToGPU)
	{
		indexCount = newIndexCount;
		Buffer::UpdateData(data, size, submitToGPU);
	}


	[[nodiscard]] uint32_t GetIndexCount() const
	{
		return indexCount;
	}

private:
	uint32_t indexCount = 0;
};

}