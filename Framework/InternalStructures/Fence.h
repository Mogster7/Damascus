//------------------------------------------------------------------------------
//
// File Name:	Fence.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/26/2020 
//
//------------------------------------------------------------------------------
#pragma once

namespace bk {

//BK_TYPE(Fence)
class Fence : public IVulkanType<vk::Fence>, public IOwned<Device>
{
	BK_TYPE_VULKAN_OWNED_BODY(Fence, IOwned<Device>)
	BK_TYPE_VULKAN_OWNED_GENERIC(Fence, Fence)
};

}




