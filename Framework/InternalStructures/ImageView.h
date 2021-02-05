//------------------------------------------------------------------------------
//
// File Name:	ImageView.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/23/2020 
//
//------------------------------------------------------------------------------
#pragma once

CUSTOM_VK_DECLARE_DERIVE(ImageView, ImageView, Device)

public:
	void CreateTexture2DView(vk::Image image, Device& owner);
};

