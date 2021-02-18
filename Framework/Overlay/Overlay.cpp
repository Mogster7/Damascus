#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"
#include "Window.h"

void Overlay::Create(std::weak_ptr<Window> window, Instance& instance, PhysicalDevice& physicalDevice, Device& device)
{
	// Allocate descriptor pools for ImGui
	uint32_t imageCount = device.swapchain.images.size();
	// How many descriptors, not descriptor sets

	vk::DescriptorPoolSize pool_sizes[] =
	{
		{ vk::DescriptorType::eSampler, 1000 },
		{ vk::DescriptorType::eCombinedImageSampler, 1000 },
		{ vk::DescriptorType::eSampledImage, 1000 },
		{ vk::DescriptorType::eStorageImage, 1000 },
		{ vk::DescriptorType::eUniformTexelBuffer, 1000 },
		{ vk::DescriptorType::eStorageTexelBuffer, 1000 },
		{ vk::DescriptorType::eUniformBuffer, 1000 },
		{ vk::DescriptorType::eStorageBuffer, 1000 },
		{ vk::DescriptorType::eUniformBufferDynamic, 1000 },
		{ vk::DescriptorType::eStorageBufferDynamic, 1000 },
		{ vk::DescriptorType::eInputAttachment, 1000 }
	};

	vk::DescriptorPoolCreateInfo pool_info = {};
	pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
	pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	DescriptorPool::Create(descriptorPool, pool_info, device);


	// Make the actual IMGUI
	m_owner = &device;
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForVulkan(static_cast<Window*>(window.lock().get())->GetHandle(), true);
	ImGui_ImplVulkan_InitInfo initInfo = {};

	initInfo.Instance = instance;
	initInfo.PhysicalDevice = physicalDevice;
	initInfo.Device = device;
	initInfo.QueueFamily = 0;
	initInfo.Queue = device.graphicsQueue;
	initInfo.PipelineCache = VK_NULL_HANDLE;
	initInfo.DescriptorPool = descriptorPool;
	initInfo.Allocator = nullptr;
	initInfo.MinImageCount = device.swapchain.images.size();
	initInfo.ImageCount = device.swapchain.images.size();
	initInfo.CheckVkResultFn = utils::AssertVkBase;

	vk::AttachmentDescription attachment = {};
	attachment.format = device.swapchain.imageFormat;
	attachment.samples = vk::SampleCountFlagBits::e1;
	attachment.loadOp = vk::AttachmentLoadOp::eLoad;
	attachment.storeOp = vk::AttachmentStoreOp::eStore;
	attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachment.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
	attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

	vk::AttachmentReference colorAttach = {};
	colorAttach.attachment = 0;
	colorAttach.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription subpass = {};
	subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttach;

	vk::SubpassDependency dependency = {};
	dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL);
	dependency.dstSubpass = 0;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

	vk::RenderPassCreateInfo createInfo = {};
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &attachment;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;
	createInfo.dependencyCount = 1;
	createInfo.pDependencies = &dependency;
	renderPass.Create(createInfo, device);

	ImGui_ImplVulkan_Init(&initInfo, renderPass.Get());


	// COMMAND POOL 
	vk::CommandPoolCreateInfo poolInfo = {};
	poolInfo.queueFamilyIndex = physicalDevice.GetQueueFamilyIndices().graphics.value();
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	commandPool.Create(commandPool, poolInfo, device);

	// COMMAND BUFFER
	commandBuffers.resize(device.swapchain.imageViews.size());

	vk::CommandBufferAllocateInfo bufferInfo = {};
	bufferInfo.level = vk::CommandBufferLevel::ePrimary;
	bufferInfo.commandPool = commandPool.Get();
	bufferInfo.commandBufferCount = commandBuffers.size();
	utils::CheckVkResult(
		device.allocateCommandBuffers(&bufferInfo, commandBuffers.data()),
		"Failed to allocate ImGui command buffers");


	auto cmdBuf = commandPool.BeginCommandBuffer();
	ImGui_ImplVulkan_CreateFontsTexture(cmdBuf.get());
	device.commandPool.EndCommandBuffer(cmdBuf.get());

	{
		vk::ImageView attachment[1];
		vk::FramebufferCreateInfo info = {};
		info.renderPass = renderPass;
		info.attachmentCount = 1;
		info.pAttachments = attachment;
		auto extent = device.swapchain.GetExtentDimensions();
		info.width = extent.x;
		info.height = extent.y;
		info.layers = 1;

		framebuffers.resize(device.swapchain.images.size());

		for (uint32_t i = 0; i < device.swapchain.images.size(); ++i)
		{
			auto view = device.swapchain.imageViews[i];
			attachment[0] = view.Get();
			framebuffers[i].Create(framebuffers[i], info, device);
		}
	}


}

void Overlay::Destroy()
{
	for (auto& framebuffer : framebuffers)
		framebuffer.Destroy();
	renderPass.Destroy();
	m_owner->freeCommandBuffers(commandPool.Get(),
								commandBuffers.size(),
								commandBuffers.data());
	commandPool.Destroy();
	descriptorPool.Destroy();
	ImGui_ImplVulkan_Shutdown();
	ImGui::DestroyContext();
}

void Overlay::RecordCommandBuffers(uint32_t imageIndex)
{
	auto extent = m_owner->swapchain.extent;

	vk::RenderPassBeginInfo info = { };
	info.renderPass = renderPass;
	info.framebuffer = framebuffers[imageIndex].Get();
	info.renderArea.extent = extent;
	info.clearValueCount = 1;
	auto clearColor = vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 1.0f, 1.0f });
	vk::ClearValue clearValue = {};
	clearValue.color = clearColor;
	info.pClearValues = &clearValue;

	// Record command buffers??ung  ??ulkan
	vk::CommandBufferBeginInfo cmdBufBeginInfo(
		{},
		nullptr
	);

	auto cmdBuf = commandBuffers[imageIndex];
	cmdBuf.begin(cmdBufBeginInfo);
	cmdBuf.beginRenderPass(info, vk::SubpassContents::eInline);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuf);
	cmdBuf.endRenderPass();
	cmdBuf.end();
}
