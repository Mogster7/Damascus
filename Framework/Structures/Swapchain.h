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
    std::vector<vk::Image> images = {};
    vk::Format imageFormat = {};
    vk::Extent2D extent = {};

    std::vector<ImageView> imageViews = {};

public:
    static void Create(Swapchain& obj, const vk::SwapchainCreateInfoKHR& createInfo,
        Device& owner, vk::Format imageFormat, vk::Extent2D extent);
    
    std::vector<vk::Image>& GetImages() { return images; }
    std::vector<ImageView>& GetImageViews() { return imageViews; }
    vk::Format GetImageFormat() const { return imageFormat; }
    vk::Extent2D GetExtent() const { return extent; } 
    glm::uvec2 GetExtentDimensions() const { return { extent.width, extent.height }; }

    void SetFormat(vk::Format imageFormat) { imageFormat = imageFormat; }
    void SetExtent(vk::Extent2D extent) { extent = extent; }

    static vk::Extent2D ChooseExtent(vk::SurfaceCapabilitiesKHR);
    static vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>&);
    static vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>&);

    void CreateImageViews();
private:

};




