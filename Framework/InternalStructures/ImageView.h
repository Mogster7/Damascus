//------------------------------------------------------------------------------
//
// File Name:	ImageView.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/23/2020 
//
//------------------------------------------------------------------------------
#pragma once

namespace dm
{

class ImageView : public IVulkanType<vk::ImageView>, public IOwned<Device>
{
public:
	DM_TYPE_VULKAN_OWNED_BODY(ImageView, IOwned<Device>)
    DM_TYPE_VULKAN_OWNED_GENERIC_FULL(ImageView, ImageView, , ImageView, ImageView);
	void CreateTexture2DView(vk::Image image, Device* owner);
};

}