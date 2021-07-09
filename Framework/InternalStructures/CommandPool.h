//------------------------------------------------------------------------------
//
// File Name:	CommandPool.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/23/2020 
//
//------------------------------------------------------------------------------
#pragma once

namespace bk {

//BK_TYPE(CommandPool)
class CommandPool : public IVulkanType<vk::CommandPool>, public IOwned<Device>
{
public:
	BK_TYPE_VULKAN_OWNED_BODY(CommandPool, IOwned<Device>)
	BK_TYPE_VULKAN_OWNED_GENERIC(CommandPool, CommandPool)
	vk::UniqueCommandBuffer BeginCommandBuffer();

	void EndCommandBuffer(vk::CommandBuffer buffer) const;

	template<class ...T>
	void FreeCommandBuffers(T&& ... args) const
	{
		std::vector <std::vector<CommandBuffer>> argVec = {std::move(args)...};
		for (auto& arg : argVec)
		{
			owner->freeCommandBuffers(
				VkType(),
				static_cast<uint32_t>(arg.size()),
				arg.data()
			);
		}
	}
};


}