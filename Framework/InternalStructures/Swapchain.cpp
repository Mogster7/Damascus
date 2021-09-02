//------------------------------------------------------------------------------
//
// File Name:	Swapchain.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/23/2020
//
//------------------------------------------------------------------------------
#include "RenderingContext.h"
#include "Window.h"
#include "Swapchain.h"


namespace dm {
void Swapchain::Create(
	const vk::SwapchainCreateInfoKHR& createInfo,
	vk::Format inImageFormat,
	vk::Extent2D inExtent,
	Device* inOwner
)
{
	IOwned::Create(inOwner);

	DM_ASSERT_VK(owner->createSwapchainKHR(&createInfo, nullptr, &VkType()));
	imageFormat = inImageFormat;
	extent = inExtent;
	images = owner->getSwapchainImagesKHR(VkType());
	CreateImageViews();
}


vk::Extent2D Swapchain::ChooseExtent(RenderingContext* context, glm::uvec2 windowDimensions, vk::SurfaceCapabilitiesKHR capabilities)
{
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	glm::uvec2 winDims = context->instance.window.lock()->GetDimensions();
	vk::Extent2D actualExtent = {winDims.x, winDims.y};

	// Clamp the capable extent to the limits defined by the program
	actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
	actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

	return actualExtent;
}

vk::PresentModeKHR Swapchain::ChoosePresentMode(const std::vector<vk::PresentModeKHR>& presentModes)
{
	//// Mailbox replaces already queued images with newer ones instead of blocking
	//// when the queue is full. Used for triple buffering
	//for (const auto& availablePresentMode : availablePresentModes) {
	//	if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
	//		return availablePresentMode;
	//	}
	//}

	// FIFO takes an image from the front of the queue and inserts it in the back
	// If queue is full then the program is blocked. Resembles v-sync
	return vk::PresentModeKHR::eFifo;
}


vk::SurfaceFormatKHR Swapchain::ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
	// If only 1 format available and is undefined, then this means ALL availableFormats are available (no restrictions)
	if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined)
	{
		return {vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
	}

	// If restricted, search for optimal format
	for (const auto& format : availableFormats)
	{
		if ((format.format == vk::Format::eR8G8B8A8Unorm ||
             format.format == vk::Format::eB8G8R8A8Unorm ||
             format.format == vk::Format::eB8G8R8Unorm   ||
             format.format == vk::Format::eR8G8B8Unorm)
			&& format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			return format;
		}
	}

	// If can't find optimal format, then just return first format
	return availableFormats[0];
}

void Swapchain::CreateImageViews()
{
	size_t imagesSize = images.size();
	imageViews.resize(imagesSize);

	for (size_t i = 0; i < imagesSize; ++i)
	{
		vk::ImageViewCreateInfo createInfo(
			{},
			images[i],
			vk::ImageViewType::e2D,
			imageFormat,
			vk::ComponentMapping(),
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
		);

		imageViews[i].Create(createInfo, owner);

		assert(bool(imageViews[i].VkType()));
	}

}

Swapchain::~Swapchain() noexcept
{
	owner->destroySwapchainKHR(VkType());
}

}
