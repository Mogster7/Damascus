#pragma once

namespace dm
{
class CommandPool;

//BK_TYPE(CommandBuffer)
class CommandBuffer : public IVulkanType<vk::CommandBuffer>
{
public:
	void Begin(vk::CommandBufferBeginInfo beginInfo = {{}, nullptr })
	{
		utils::CheckVkResult(
			begin(&beginInfo),
			"Failed to begin recording command buffer"
		);
	}

	void End()
	{
		end();
	}
};

class CommandBufferVector : public IOwned<CommandPool>, public std::vector<CommandBuffer>
{
public:
	void Create(const vk::CommandBufferAllocateInfo& commandBufferAllocateInfo, CommandPool* inOwner);
	~CommandBufferVector() noexcept;
};

}