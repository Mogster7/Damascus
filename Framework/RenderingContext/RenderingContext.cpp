#include "RenderingContext.h"
#include "Window.h"
#include "Overlay/Overlay.h"
#include <glfw3.h>
#include <vk_mem_alloc.h>


VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
namespace bk {


void RenderingContext::Update()
{
	static float prevTime = glfwGetTime();
	float time = glfwGetTime();
	dt = time - prevTime;

	device.Update(dt);
	prevTime = time;
}

void RenderingContext::Destroy()
{
	DestroyMeshStatics();
	device.waitIdle();

	device.freeCommandBuffers(commandPool.VkType(),
							  drawBuffers.size(),
							  drawBuffers.data());
}


void RenderingContext::CreateRenderPass()
{
	// ATTACHMENTS
	vk::AttachmentDescription colorAttachment;
	colorAttachment.format = swapchain.imageFormat;
	colorAttachment.samples = vk::SampleCountFlagBits::e1;                    // Number of samples to write for multisampling
	colorAttachment.loadOp = vk::AttachmentLoadOp::eLoad;                // Describes what to do with attachment before rendering
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;            // Describes what to do with attachment after rendering
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;    // Describes what to do with stencil before rendering
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;    // Describes what to do with stencil after rendering

	// Framebuffer data will be stored as an image, but images can be given different data layouts
	// to give optimal use for certain operations
	colorAttachment.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;            // Image data layout before render pass starts
	colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;        // Image data layout after render pass (to change to)

	// Depth
	vk::AttachmentDescription depthAttachment = {};
	depthAttachment.format = depth.format;
	depthAttachment.samples = vk::SampleCountFlagBits::e1;
	depthAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
	depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachment.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;


	std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	// REFERENCES
	// Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
	vk::AttachmentReference colorAttachmentRef;
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	// Depth
	vk::AttachmentReference depthAttachmentRef;
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;


	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	vk::SubpassDependency dependency({});

	// ---------------------------
	// SUBPASS for converting between IMAGE_LAYOUT_UNDEFINED to IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL

	// Indices of the dependency for this subpass and the dependent subpass
	// VK_SUBPASS_EXTERNAL refers to the implicit subpass before/after this pass
	// Happens after
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	// Pipeline Stage
	dependency.srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
	// Stage access mask
	dependency.srcAccessMask = vk::AccessFlagBits::eMemoryRead;

	// Happens before
	dependency.dstSubpass = 0;
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
	dependency.dependencyFlags = {};

	// Operations to wait on and what stage they occur
// Wait on swap chain to finish reading from the image to access it
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.srcAccessMask = {};

	// Operations that should wait on the above operations to finish are in the color attachment writing stage
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

	vk::RenderPassCreateInfo createInfo;
	createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	createInfo.pAttachments = attachments.data();
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;
	createInfo.dependencyCount = 1;
	createInfo.pDependencies = &dependency;

	std::vector<vk::ClearValue> clearValues(2);
	clearValues[0].color = vk::ClearColorValue(std::array<float, 4>{ 1.0f, 1.0f, 1.0f, 1.0f });
	clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f);

	renderPass.Create(createInfo, swapchain.extent, clearValues, &device);
}

void RenderingContext::CreateSync()
{
	imageAvailable = std::vector<Semaphore>(MAX_FRAME_DRAWS);
	renderFinished = std::vector<Semaphore>(MAX_FRAME_DRAWS);
	drawFences = std::vector<Fence>(MAX_FRAME_DRAWS);

	vk::SemaphoreCreateInfo semInfo{};
	vk::FenceCreateInfo fenceInfo{};
	fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

	for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
	{
		imageAvailable[i].Create(semInfo, &device);
		renderFinished[i].Create(semInfo, &device);
		drawFences[i].Create(fenceInfo, &device);
	}
}

void RenderingContext::CreateFramebuffers(bool recreate /*= false*/)
{
//	if (recreate)
//	{
//		for (auto& fb : frameBuffers)
//			fb.Destroy();
//	}

	const auto& imageViews = swapchain.imageViews;
	const auto& extent = swapchain.extent;
	size_t imageViewsSize = imageViews.size();
	frameBuffers.resize(imageViewsSize);

	vk::FramebufferCreateInfo createInfo;
	// FB width/height
	createInfo.width = extent.width;
	createInfo.height = extent.height;
	// FB layers
	createInfo.layers = 1;
	createInfo.renderPass = renderPass.VkType();
	createInfo.attachmentCount = 2;


	vk::ImageView depthView = depth.imageView.VkType();
	for (size_t i = 0; i < imageViewsSize; ++i)
	{
		std::array<vk::ImageView, 2> attachments = {
			imageViews[i].VkType(),
			depthView
		};

		// List of attachments 1 to 1 with render pass
		createInfo.pAttachments = attachments.data();

		frameBuffers[i].Create(createInfo, &device);
	}
}

void RenderingContext::CreateDepthBuffer(bool recreate /*= false*/)
{
//	if (recreate)
//		depth.Destroy();

	auto& extent = swapchain.extent;
	depth.Create(FrameBufferAttachment::GetDepthFormat(), { extent.width, extent.height },
		vk::ImageUsageFlagBits::eDepthStencilAttachment |
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc,
		vk::ImageAspectFlagBits::eDepth,
		//vk::ImageLayout::eTransferSrcOptimal,
		vk::ImageLayout::eDepthStencilAttachmentOptimal,

		// No sampler
		&device
	);
}

void RenderingContext::CreateCommandBuffers(bool recreate)
{
//	if (recreate)
//	{
//		device.freeCommandBuffers(commandPool.VkType(),
//			drawBuffers.size(),
//			drawBuffers.data());
//	}

	size_t fbSize = frameBuffers.size();
	drawBuffers.resize(fbSize);

	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.commandPool = commandPool.VkType();
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandBufferCount = static_cast<uint32_t>(fbSize);

	utils::CheckVkResult(device.allocateCommandBuffers(&allocInfo, drawBuffers.data()), "Failed to allocate command buffers");
}

void RenderingContext::CreateCommandPool()
{
	QueueFamilyIndices indices = physicalDevice.GetQueueFamilyIndices();

	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	poolInfo.queueFamilyIndex = indices.graphics.value();

	commandPool.Create(poolInfo, &device);
}

void RenderingContext::CreateSwapchain(bool recreate)
{
//	if (recreate)
//		swapchain.Destroy();

	vk::SurfaceKHR surface = instance.surface;

	vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
	std::vector<vk::SurfaceFormatKHR> formats = physicalDevice.getSurfaceFormatsKHR(surface);
	std::vector<vk::PresentModeKHR> presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

	// Surface format (color depth)
	vk::SurfaceFormatKHR format = Swapchain::ChooseSurfaceFormat(formats);
	// Presentation mode (conditions for swapping images)
	vk::PresentModeKHR presentMode = Swapchain::ChoosePresentMode(presentModes);
	// Swap extent (resolution of images in the swap chain)
	vk::Extent2D extent = Swapchain::ChooseExtent(instance.window.lock()->GetDimensions(),
		capabilities);

	// 1 more than the min to not wait on acquiring
	uint32_t imageCount = capabilities.minImageCount + 1;
	// Clamp to max if greater than 0
	if (capabilities.maxImageCount > 0 &&
		imageCount > capabilities.maxImageCount)
	{
		imageCount = capabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR createInfo(
		{},
		instance.surface,
		imageCount,
		format.format,
		format.colorSpace,
		extent,
		1,
		vk::ImageUsageFlagBits::eColorAttachment
	);


	const QueueFamilyIndices& indices = physicalDevice.GetQueueFamilyIndices();
	uint32_t queueFamilyIndices[] = { indices.graphics.value(), indices.present.value() };

	if (indices.graphics != indices.present)
	{
		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	// Old swap chain is default
	swapchain.Create(createInfo, format.format, extent, &device);
}


bool RenderingContext::PrepareFrame(const uint32_t frameIndex)
{
	auto result = device.acquireNextImageKHR(swapchain.VkType(),
		std::numeric_limits<uint64_t>::max(),
		imageAvailable[frameIndex].VkType(),
		nullptr, &imageIndex);
	// Freeze until previous image is drawn
	utils::CheckVkResult(device.waitForFences(1, &drawFences[frameIndex], VK_TRUE, std::numeric_limits<uint64_t>::max()),
		"Failed on waiting for fences");
	// Close the fence for this current frame again
	utils::CheckVkResult(device.resetFences(1, &drawFences[frameIndex]),
		"Failed to reset fences");
	// Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
	if ((result == vk::Result::eErrorOutOfDateKHR) || (result == vk::Result::eSuboptimalKHR))
	{

		// Return if surface is out of date
		return true;
	}
	else
	{
		utils::CheckVkResult(result, "Failed to prepare render frame");
		return false;
	}
}

bool RenderingContext::SubmitFrame(const uint32_t frameIndex, const vk::Semaphore* wait, uint32_t waitCount)
{
	vk::PresentInfoKHR presentInfo{};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pWaitSemaphores = wait;
	presentInfo.waitSemaphoreCount = waitCount;
	vk::Result result = device.graphicsQueue.presentKHR(presentInfo);

	if (!((result == vk::Result::eSuccess) || (result == vk::Result::eSuboptimalKHR)))
	{
		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			// Return if surface is out of date
			// Swap chain is no longer compatible with the surface and needs to be recreated
			return true;
		}
		else
		{
			utils::CheckVkResult(result, "Failed to submit frame");
		}
	}
	return false;
}

void RenderingContext::CreateLogicalDevice()
{
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> queueFamilies = { physicalDevice.queueFamilyIndices.graphics.value(), physicalDevice.queueFamilyIndices.present.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : queueFamilies)
	{
		queueCreateInfos.emplace_back(
			vk::DeviceQueueCreateFlags(),
			queueFamily,
			1,
			&queuePriority
		);
	}

	vk::PhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.wideLines = VK_TRUE;
	deviceFeatures.fillModeNonSolid = VK_TRUE;

	vk::DeviceCreateInfo createInfo(
		{},
		(uint32_t) queueCreateInfos.size(),
		queueCreateInfos.data(),
		0,
		{},
		(uint32_t) deviceExtensions.size(),
		deviceExtensions.data(),
		&deviceFeatures
	);

	device.Create(createInfo, &physicalDevice);
}


void RenderingContext::Create(std::weak_ptr<Window> window, bool enabledOverlay)
{
	instance.Create();
	instance.CreateSurface(window);
	physicalDevice.Create(this);

	// Initialize context variables
	CreateLogicalDevice();
	CreateCommandPool();
	CreateSwapchain();
	CreateDepthBuffer();
	CreateRenderPass();
	CreateFramebuffers();
	CreateCommandBuffers();
	CreateSync();

	InitializeMeshStatics(&device);
	if (enabledOverlay) {
		overlay = std::make_unique<Overlay>();
		overlay->Create(window, &device);
	}
}


}