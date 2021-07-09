#pragma once

namespace bk {

//BK_TYPE(CommandBuffer)
class CommandBuffer : public vk::CommandBuffer
{
public:
	void Begin(vk::CommandBufferBeginInfo beginInfo = {{}, nullptr})
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

	vk::CommandBuffer& Get()
	{
		return *this;
	}
};

}