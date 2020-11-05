//------------------------------------------------------------------------------
//
// File Name:	DepthBuffer.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        7/25/2020 
//
//------------------------------------------------------------------------------
#pragma once
class Device;

class DepthBuffer : public eastl::safe_object
{
    Image m_Image;
    ImageView m_ImageView;
    eastl::safe_ptr<Device> m_Owner;
    vk::Format m_Format;

    static vk::Format ChooseFormat(const eastl::vector<vk::Format>& formats, 
        vk::ImageTiling tiling, vk::FormatFeatureFlagBits features);
public:
    void Create(Device& owner);
    void Destroy();
    
    vk::Format GetFormat() const { return m_Format; }
    ImageView& GetImageView() { return m_ImageView; }
};

