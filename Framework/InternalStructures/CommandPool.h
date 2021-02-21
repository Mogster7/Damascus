//------------------------------------------------------------------------------
//
// File Name:	CommandPool.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/23/2020 
//
//------------------------------------------------------------------------------
#pragma once


CUSTOM_VK_DECLARE_DERIVE(CommandPool, CommandPool, Device)
    
public:
	vk::UniqueCommandBuffer BeginCommandBuffer();
	void EndCommandBuffer(vk::CommandBuffer buffer) const;

	template <class ...T>
	void FreeCommandBuffers(T&&... args) const
	{
		std::vector<std::vector<CommandBuffer>> argVec = { std::move(args)... };
		for (auto& arg : argVec)
		{
			m_owner->freeCommandBuffers(Get(),
										static_cast<uint32_t>(arg.size()),
										arg.data());
		}
	}
};




