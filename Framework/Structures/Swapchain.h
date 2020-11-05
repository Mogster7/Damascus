//------------------------------------------------------------------------------
//
// File Name:	Swapchain.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/23/2020 
//
//------------------------------------------------------------------------------
#pragma once


CUSTOM_VK_DECLARE_DERIVE_KHR(Swapchain, Swapchain, Device)

    // Uses same index as framebuffer & command buffer
    std::vector<vk::Image> m_Images = {};
    vk::Format m_ImageFormat = {};
    vk::Extent2D m_Extent = {};

    std::vector<ImageView> m_ImageViews = {};

public:
    static void Create(Swapchain& obj, const vk::SwapchainCreateInfoKHR& createInfo,
        Device& owner, vk::Format imageFormat, vk::Extent2D extent);
    
    std::vector<vk::Image>& GetImages() { return m_Images; }
    std::vector<ImageView>& GetImageViews() { return m_ImageViews; }
    vk::Format GetImageFormat() const { return m_ImageFormat; }
    vk::Extent2D GetExtent() const { return m_Extent; } 
    glm::uvec2 GetExtentDimensions() const { return { m_Extent.width, m_Extent.height }; }

    void SetFormat(vk::Format imageFormat) { m_ImageFormat = imageFormat; }
    void SetExtent(vk::Extent2D extent) { m_Extent = extent; }

    static vk::Extent2D ChooseExtent(vk::SurfaceCapabilitiesKHR);
    static vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>&);
    static vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>&);

    void CreateImageViews();
private:

};




