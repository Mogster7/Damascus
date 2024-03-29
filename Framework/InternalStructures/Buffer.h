//------------------------------------------------------------------------------
//
// File Name:	Buffer.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/9/2020 
//
//------------------------------------------------------------------------------
#pragma once
namespace dm
{

class Device;

class Buffer : public IVulkanType<vk::Buffer, VkBuffer>, public IOwned<Device>
{
public:
DM_TYPE_VULKAN_OWNED_BODY(Buffer, IOwned < Device >)
	Buffer(Buffer&& other) noexcept;
	Buffer& operator=(Buffer&& other) noexcept;
	~Buffer() noexcept override;

	void Create(
		const vk::BufferCreateInfo& bufferCreateInfo,
		const VmaAllocationCreateInfo& allocCreateInfo,
		Device* inOwner
	);

    void Destroy();

	void CreateStaged(
		void* data,
		vk::DeviceSize size,
		vk::BufferUsageFlags bufferUsage,
		VmaMemoryUsage memoryUsage,
		bool submitToGPU,
		bool inPersistentMapped,
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
    void StageTransferDynamicSingleSubmit();

	static std::vector<vk::DescriptorBufferInfo*> AggregateDescriptorInfo(std::vector<Buffer>& buffers);

	VmaAllocation allocation = {};
	bool persistentMapped = false;
    bool dirty = false;
	VmaAllocationInfo allocationInfo = {};
	vk::DescriptorBufferInfo descriptorInfo = {};
	std::shared_ptr<Buffer> stagingBuffer = {};

	// Save for copy construction/destruction
	VmaAllocationCreateInfo allocationCI = {};
	vk::BufferCreateInfo bufferCI = {};

	static void Map(Buffer& buffer, void* data);

private:
};

template<class VertexType>
class VertexBuffer : public Buffer
{
public:
DM_TYPE_OWNED_BODY(VertexBuffer<VertexType>, Buffer)
	VertexBuffer(VertexBuffer&& other) noexcept = default;
	VertexBuffer& operator=(VertexBuffer&& other) noexcept = default;
	~VertexBuffer() noexcept override = default;

	void Create(const std::vector<VertexType>& vertices, bool dynamic, Device* owner)
	{
		assert(!vertices.empty());
		vertexCount = vertices.size();
		Buffer::CreateStaged(
			(void*) &vertices[0], vertices.size() * sizeof(VertexType),
			vk::BufferUsageFlagBits::eVertexBuffer,
			VMA_MEMORY_USAGE_GPU_ONLY,
			!dynamic,
			dynamic,
			owner
		);
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


class IndexBuffer : public Buffer
{
public:
DM_TYPE_OWNED_BODY(IndexBuffer, Buffer)
	IndexBuffer& operator=(IndexBuffer&& other) noexcept = default;
	IndexBuffer(IndexBuffer&& other) noexcept = default;
	~IndexBuffer() noexcept override = default;

	void Create(const std::vector<uint32_t>& indices, bool dynamic, Device* owner)
	{
		assert(!indices.empty());
		indexCount = indices.size();
		Buffer::CreateStaged(
			(void*) indices.data(), indices.size() * sizeof(uint32_t),
			vk::BufferUsageFlagBits::eIndexBuffer,
			VMA_MEMORY_USAGE_GPU_ONLY,
			!dynamic,
			dynamic,
			owner);
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

template <class T, size_t N>
struct InstancedBuffer : public Buffer
{
    DM_TYPE_OWNED_BODY(InstancedBuffer, Buffer)
    InstancedBuffer& operator=(InstancedBuffer&& other) noexcept = default;
    InstancedBuffer(InstancedBuffer&& other) noexcept = default;
    ~InstancedBuffer() noexcept override = default;

    void CreateStaged(Device* owner)
    {
        static_assert(N != 0);
        Buffer::CreateStaged(
            nullptr,
            sizeof(T) * N,
            vk::BufferUsageFlagBits::eVertexBuffer,
            VMA_MEMORY_USAGE_GPU_ONLY,
            false,
            true,
            owner);
    }


    T* Data()
    {
       return reinterpret_cast<T*>(allocationInfo.pMappedData);
    }

    constexpr size_t Count() { return N; }
    constexpr size_t MemorySize() { return sizeof(T) * N; };
};

}