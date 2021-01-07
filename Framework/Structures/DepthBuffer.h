//------------------------------------------------------------------------------
//
// File Name:	DepthBuffer.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        7/25/2020 
//
//------------------------------------------------------------------------------
#pragma once
class Device;

class DepthBuffer 
{
	Device* m_owner = nullptr;

    static vk::Format ChooseFormat(const std::vector<vk::Format>& formats, 
        vk::ImageTiling tiling, vk::FormatFeatureFlagBits features);

public:
    void Create(Device& owner);
    void Destroy();

	Image image;
    ImageView imageView;
    vk::Format format;

    vk::Format GetFormat() const { return format; }
    ImageView& GetImageView() { return imageView; }
};

