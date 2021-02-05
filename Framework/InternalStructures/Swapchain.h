//------------------------------------------------------------------------------
//
// File Name:	Swapchain.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/23/2020 
//
//------------------------------------------------------------------------------
#pragma once


CUSTOM_VK_DECLARE_DERIVE_KHR(Swapchain, Swapchain, Device)



public:
    static void Create(Swapchain& obj, const vk::SwapchainCreateInfoKHR& createInfo,
        Device& owner, vk::Format imageFormat, vk::Extent2D extent);


    static vk::Extent2D ChooseExtent(vk::SurfaceCapabilitiesKHR);
    static vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>&);
    static vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>&);

    void CreateImageViews();

    // Uses same index as framebuffer & command buffer
    std::vector<vk::Image> images = {};
    vk::Format imageFormat = {};
    vk::Extent2D extent = {};
    std::vector<ImageView> imageViews = {};

    glm::uvec2 GetExtentDimensions() const { return { extent.width, extent.height }; }


private:

};




