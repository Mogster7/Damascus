//------------------------------------------------------------------------------
//
// File Name:	DepthBuffer.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        7/25/2020 
//
//------------------------------------------------------------------------------
#pragma once
class Device;

class FrameBufferAttachment 
{
	Device* m_owner = nullptr;


public:
	void FrameBufferAttachment::Create(vk::Format format,
									   vk::Extent2D extent,
									   vk::ImageUsageFlags usage,
									   vk::ImageLayout destinationLayout,
									   Device& owner);
    void CreateDepth(Device& owner);
    void Destroy();

	Image image;
    ImageView imageView;
    vk::Format format;

    vk::Format GetFormat() const { return format; }
    ImageView& GetImageView() { return imageView; }

    static vk::Format GetDepthFormat();
};

