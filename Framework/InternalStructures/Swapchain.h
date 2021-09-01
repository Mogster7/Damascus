//------------------------------------------------------------------------------
//
// File Name:	Swapchain.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/23/2020 
//
//------------------------------------------------------------------------------
#pragma once
namespace dm {

//DM_TYPE(Swapchain)
class Swapchain : public IVulkanType<vk::SwapchainKHR>, public IOwned<Device>
{
public:
DM_TYPE_VULKAN_OWNED_BODY(Swapchain, IOwned<Device>)

	void Create(
		const vk::SwapchainCreateInfoKHR& createInfo,
		vk::Format imageFormat,
		vk::Extent2D extent,
		Device* owner
	);

	~Swapchain() noexcept;

	static vk::Extent2D ChooseExtent(RenderingContext* context, glm::uvec2 windowDimensions, vk::SurfaceCapabilitiesKHR);

	static vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>&);

	static vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>&);


	// Uses same index as framebuffer & command buffer
	std::vector<vk::Image> images = {};
	std::vector<ImageView> imageViews = {};
	vk::Format imageFormat = {};
	vk::Extent2D extent = {};

	[[nodiscard]] glm::uvec2 GetExtentDimensions() const
	{
		return {extent.width, extent.height};
	}


private:
	void CreateImageViews();
};

}