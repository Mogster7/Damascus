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
};




