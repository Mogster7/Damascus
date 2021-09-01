//------------------------------------------------------------------------------
//
// File Name:	Framebuffer.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/23/2020 
//
//------------------------------------------------------------------------------
#pragma once
namespace dm {

////BK_TYPE(FrameBuffer)
class FrameBuffer : public IVulkanType<vk::Framebuffer>, public IOwned<Device>
{
	BK_TYPE_VULKAN_OWNED_BODY(FrameBuffer, IOwned<Device>)
	BK_TYPE_VULKAN_OWNED_GENERIC(FrameBuffer, Framebuffer)
};


}