#pragma once

namespace dm
{
class CommandPool;

//DM_TYPE(CommandBuffer)
class CommandBuffer : public IVulkanType<vk::CommandBuffer>
{
public:
	void Begin(vk::CommandBufferBeginInfo beginInfo = {{}, nullptr })
	{
		DM_ASSERT_VK(
			begin(&beginInfo)
		);
	}

	void End()
	{
		end();
	}
};

class CommandBufferVector : public IOwned<CommandPool>
{
public:
	void Create(const vk::CommandBufferAllocateInfo& commandBufferAllocateInfo, CommandPool* inOwner);
    void Destroy();
    [[nodiscard]] size_t Size() const { return commandBuffers.size(); }
    [[nodiscard]] bool IsEmpty() const { return commandBuffers.empty(); }
    [[nodiscard]] vk::CommandBuffer* Data() { return commandBuffers.data(); }
    [[nodiscard]] vk::CommandBuffer& operator[](int index) { return commandBuffers[index]; }

	~CommandBufferVector() noexcept override;

    ImageAsync<vk::CommandBuffer> commandBuffers;
};

}