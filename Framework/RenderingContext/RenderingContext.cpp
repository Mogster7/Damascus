#include "RenderingContext.h"
#include "Window.h"
#include <SDL.h>
#include <vk_mem_alloc.h>


VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
namespace dm
{


void RenderingContext::Update(float dt)
{
	device.Update(dt);
}

void RenderingContext::Destroy()
{
	device.waitIdle();
  DestroyMeshStatics();

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
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;                // Describes what to do with attachment before rendering
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;            // Describes what to do with attachment after rendering
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;    // Describes what to do with stencil before rendering
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;    // Describes what to do with stencil after rendering

	// Framebuffer data will be stored as an image, but images can be given different data layouts
	// to give optimal use for certain operations
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;            // Image data layout before render pass starts
	colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;        // Image data layout after render pass (to change to)

	// Depth
	vk::AttachmentDescription depthAttachment = {};
	depthAttachment.format = depth.format;
	depthAttachment.samples = vk::SampleCountFlagBits::e1;
	depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
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
	depth.Create(FrameBufferAttachment::GetDepthFormat(this), { extent.width, extent.height },
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
	vk::Extent2D extent = Swapchain::ChooseExtent(this, instance.window.lock()->GetDimensions(),
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

bool RenderingContext::SubmitFrame(const vk::Semaphore* wait, uint32_t waitCount)
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


void RenderingContext::Create(std::weak_ptr<Window> window)
{
	instance.Create(window);
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
	CreateForwardPipeline();

	InitializeMeshStatics(&device);
}

float RenderingContext::UpdateDeltaTime()
{
  // Milliseconds
	static uint32_t prevTime = 0;
	uint32_t time = SDL_GetTicks();

	float dt = static_cast<float>(time - prevTime) * 0.001f;

	prevTime = time;
	return dt;
}

void RenderingContext::CreateForwardPipeline()
{
	// FORWARD PASS
	// ---------
	// ATTACHMENTS
	vk::AttachmentDescription colorAttachment;
	colorAttachment.format = swapchain.imageFormat;
	colorAttachment.samples = vk::SampleCountFlagBits::e1;                    // Number of samples to write for multisampling
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;                // Describes what to do with attachment before rendering
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;            // Describes what to do with attachment after rendering
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;    // Describes what to do with stencil before rendering
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;    // Describes what to do with stencil after rendering

	// Framebuffer data will be stored as an image, but images can be given different data layouts
	// to give optimal use for certain operations
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;            // Image data layout before render pass starts
	colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;        // Image data layout after render pass (to change to)

	// Depth
	vk::AttachmentDescription depthAttachment = {};
	depthAttachment.format = depth.format;
	depthAttachment.samples = vk::SampleCountFlagBits::e1;
	depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
	depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	auto sampled = vk::ImageUsageFlagBits::eSampled;

	forwardPass.depth.CreateDepth(&device);

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

	vk::RenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassCreateInfo.pAttachments = attachments.data();
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &dependency;




	// DESCRIPTORS FOR PIPELINE
	// ----------------------o-
	struct UboViewProjection {
		glm::mat4 projection;
		glm::mat4 view;
	} uboViewProjection;

	vk::DeviceSize vpBufferSize = sizeof(UboViewProjection);

	const auto& images = swapchain.images;
	// One uniform buffer for each image
	uniformBufferViewProjection.resize(images.size());

	vk::BufferCreateInfo vpCreateInfo = {};
	vpCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	vpCreateInfo.size = vpBufferSize;


	VmaAllocationCreateInfo aCreateInfo = {};
	aCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	for (size_t i = 0; i < images.size(); ++i) {
		uniformBufferViewProjection[i].Create(vpCreateInfo, aCreateInfo, &device);
	}

	auto vpDescInfos = Buffer::AggregateDescriptorInfo(uniformBufferViewProjection);

	std::array<WriteDescriptorSet, 1> descriptorInfos =
		{
			WriteDescriptorSet::CreateAsync(0,
				vk::ShaderStageFlagBits::eVertex,
				vk::DescriptorType::eUniformBuffer,
				vpDescInfos),
		};

	descriptors.Create<1>(
		descriptorInfos,
		swapchain.images.size(),
		&device
	);

	// PUSH CONSTANTS
	pushRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
	pushRange.offset = 0;
	pushRange.size = sizeof(glm::mat4);

	// Command buffer allocate info
	vk::CommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.commandPool = commandPool.VkType();
	commandBufferAllocateInfo.commandBufferCount = GetImageViewCount();
	commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;

	// PIPELINE CREATION
	// ----------------
	ShaderModule vertModule = {};
	auto vertInfo = vertModule.Load(
		"Assets/Shaders/forwardVert.spv",
		vk::ShaderStageFlagBits::eVertex,
		&device
	);
	ShaderModule fragModule = {};
	auto fragInfo = fragModule.Load(
		"Assets/Shaders/forwardFrag.spv",
		vk::ShaderStageFlagBits::eFragment,
		&device
	);

	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertInfo, fragInfo };

	// VERTEX BINDING DATA
	vk::VertexInputBindingDescription bindDesc = {};
	// Binding position (can bind multiple streams)
	bindDesc.binding = 0;
	bindDesc.stride = sizeof(Vertex);
	// Instancing option, draw one object at a time in this case
	bindDesc.inputRate = vk::VertexInputRate::eVertex;

	// VERTEX ATTRIB DATA
	std::array<vk::VertexInputAttributeDescription, Vertex::NUM_ATTRIBS> attribDesc;
	attribDesc[0].binding = 0;
	attribDesc[0].location = 0;
	attribDesc[0].format = vk::Format::eR32G32B32Sfloat;
	attribDesc[0].offset = offsetof(Vertex, pos);
	// Normal attribute
	attribDesc[1].binding = 0;
	attribDesc[1].location = 1;
	attribDesc[1].format = vk::Format::eR32G32B32Sfloat;
	attribDesc[1].offset = offsetof(Vertex, normal);
	// Color attribute
	attribDesc[2].binding = 0;
	attribDesc[2].location = 2;
	attribDesc[2].format = vk::Format::eR32G32B32Sfloat;
	attribDesc[2].offset = offsetof(Vertex, color);
	// Texture coord
	attribDesc[3].binding = 0;
	attribDesc[3].location = 3;
	attribDesc[3].format = vk::Format::eR32G32Sfloat;
	attribDesc[3].offset = offsetof(Vertex, texPos);

	// VERTEX INPUT
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindDesc;
	// Data spacing / stride info
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDesc.size());
	vertexInputInfo.pVertexAttributeDescriptions = attribDesc.data();

	// INPUT ASSEMBLY
	vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// VIEWPORT
	vk::Extent2D extent = { swapchain.extent.width, swapchain.extent.height};

	vk::Viewport viewport{};
	viewport.x = 0.0f;
	viewport.y = extent.height;
	viewport.width = (float) extent.width;
	viewport.height = -((float) extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor;
	scissor.extent = extent;
	scissor.offset = vk::Offset2D(0, 0);

	vk::PipelineViewportStateCreateInfo viewportState = {};
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	vk::PipelineRasterizationStateCreateInfo rasterState = {};
	rasterState.depthClampEnable = VK_FALSE;            // Change if fragments beyond near/far planes are clipped (default) or clamped to plane
	rasterState.rasterizerDiscardEnable = VK_FALSE;    // Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
	rasterState.polygonMode = vk::PolygonMode::eFill;    // How to handle filling points between vertices
	rasterState.lineWidth = 1.0f;                        // How thick lines should be when drawn
	rasterState.cullMode = vk::CullModeFlagBits::eNone;        // Which face of a triangle to cull
	rasterState.frontFace = vk::FrontFace::eCounterClockwise;    // Winding to determine which side is front
	rasterState.depthBiasEnable = VK_FALSE;            // Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)

	vk::PipelineMultisampleStateCreateInfo multisampleState = {};
	// Whether or not to multi-sample
	multisampleState.sampleShadingEnable = VK_FALSE;
	// Number of samples per fragment
	multisampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;

	vk::PipelineColorBlendAttachmentState blendState = {};
	blendState.colorWriteMask = vk::ColorComponentFlags(
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT);
	blendState.blendEnable = VK_FALSE;
	std::array<vk::PipelineColorBlendAttachmentState, 1> blendAttachmentStates = {
		blendState
	};
	vk::PipelineColorBlendStateCreateInfo colorBlendState = {};
	colorBlendState.logicOpEnable = VK_FALSE;
	colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
	colorBlendState.pAttachments = blendAttachmentStates.data();

	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &descriptors.layout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushRange;

	// Depth testing
	vk::PipelineDepthStencilStateCreateInfo depthStencilState = {};
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = vk::CompareOp::eLess;
	// if this was true, you can fill out min/max bounds in this createinfo
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.stencilTestEnable = VK_FALSE;


	vk::GraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.stageCount = 2;                                    // Number of shader stages
	pipelineInfo.pStages = shaderStages;                            // List of shader stages
	pipelineInfo.pVertexInputState = &vertexInputInfo;        // All the fixed function pipeline states
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.pRasterizationState = &rasterState;
	pipelineInfo.pMultisampleState = &multisampleState;
	pipelineInfo.pColorBlendState = &colorBlendState;
	pipelineInfo.pDepthStencilState = &depthStencilState;
	pipelineInfo.subpass = 0;                                        // Subpass of render pass to use with pipeline

	vk::SemaphoreCreateInfo semaphoreCreateInfo = {};

	// FRAMEBUFFER INFO
	vk::FramebufferCreateInfo frameBufferCreateInfo;
	// FB width/height
	frameBufferCreateInfo.width = swapchain.extent.width;
	frameBufferCreateInfo.height = swapchain.extent.height;
	// FB layers
	frameBufferCreateInfo.layers = 1;
	frameBufferCreateInfo.attachmentCount = 2;

	const size_t imageViewCount = GetImageViewCount();

	std::vector<std::array<vk::ImageView, 2>> attachmentsPerImage;
	for(size_t i = 0; i < imageViewCount; ++i)
	{
		attachmentsPerImage.push_back({ swapchain.imageViews[i].VkType(), forwardPass.depth.imageView.VkType() });
	}

	std::vector<vk::ClearValue> clearValues =
		{
			vk::ClearColorValue(defaultClearColor),
			vk::ClearDepthStencilValue(defaultClearDepth)
		};

	forwardPass.Create(
		commandBufferAllocateInfo,
		renderPassCreateInfo,
		pipelineLayoutCreateInfo,
		pipelineInfo,
		semaphoreCreateInfo,
		frameBufferCreateInfo,
		swapchain.extent,
		clearValues,
		attachmentsPerImage,
		imageViewCount,
		&commandPool,
		&device
		);
}


}