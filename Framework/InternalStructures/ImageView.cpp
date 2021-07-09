//------------------------------------------------------------------------------
//
// File Name:	ImageView.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/23/2020
//
//------------------------------------------------------------------------------
#include "ImageView.h"

namespace bk {

void ImageView::CreateTexture2DView(vk::Image image, Device* owner)
{
	vk::ImageViewCreateInfo CI = {};
	CI.image = image;
	CI.viewType = vk::ImageViewType::e2D;
	CI.format = vk::Format::eR8G8B8A8Unorm;
	CI.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	CI.subresourceRange.baseMipLevel = 0;
	CI.subresourceRange.levelCount = 1;
	CI.subresourceRange.baseArrayLayer = 0;
	CI.subresourceRange.layerCount = 1;

	Create(CI, owner);
}


}