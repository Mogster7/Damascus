//------------------------------------------------------------------------------
//
// File Name:	Device.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/18/2020
//
//------------------------------------------------------------------------------
#include "RenderingContext.h"
#include "Window.h"
#include <glm/gtc/matrix_transform.hpp>

#define VMA_IMPLEMENTATION
#define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#include <vk_mem_alloc.h>



void Device::Initialization()
{
    const QueueFamilyIndices& indices = m_owner->GetQueueFamilyIndices();
    getQueue(indices.graphics.value(), 0, &graphicsQueue);
    getQueue(indices.present.value(), 0, &presentQueue);
    //minUniformBufferOffset = owner->GetMinimumUniformBufferOffset();
    //// Calculate alignment of model uniform 
    //modelUniformAlignment = (sizeof(Model) + minUniformBufferOffset - 1)
    //                            & ~(minUniformBufferOffset - 1);

    CreateAllocator();
    CreateSwapchain();
    CreateCommandPool();
    CreateDepthBuffer();
    CreateRenderPass();
    // CreateDescriptorSetLayout();
    // CreatePushConstantRange();
    CreateGraphicsPipeline();
    CreateFramebuffers();


    // CreateUniformBuffers();
    // AllocateDynamicBufferTransferSpace();
    // CreateDescriptorPool();
    // CreateDescriptorSets();

    CreateCommandBuffers();
    CreateSync();
}

void Device::CreateAllocator()
{
    ASSERT(allocator == nullptr, "Creating an existing member");
    VmaAllocatorCreateInfo info = {};
    info.instance = RenderingContext::GetInstance();
    info.physicalDevice = RenderingContext::GetPhysicalDevice();
    info.device = *this;

    vmaCreateAllocator(&info, &allocator);
}

void Device::Destroy()
{

    for(auto& fence : drawFences)
        fence.Destroy();

    for (size_t i = 0; i < renderFinished.size(); ++i)
    {
        renderFinished[i].Destroy();
        imageAvailable[i].Destroy();
    }

	freeCommandBuffers(commandPool.Get(),
					   drawCmdBuffers.size(),
					   drawCmdBuffers.data());
    commandPool.Destroy();
    for (auto& fb : framebuffers)
        fb.Destroy();

    graphicsPipeline.Destroy();
    pipelineLayout.Destroy();

    renderPass.Destroy();
    depthBuffer.Destroy();
    swapchain.Destroy();
    vmaDestroyAllocator(allocator);

    destroy();
}

void Device::CreateSwapchain(bool recreate)
{    
	if (recreate) swapchain.Destroy();
	
    vk::PhysicalDevice physDevice = *m_owner;
    vk::SurfaceKHR surface = RenderingContext::GetInstance().GetSurface();

    vk::SurfaceCapabilitiesKHR capabilities = physDevice.getSurfaceCapabilitiesKHR(surface);
    std::vector<vk::SurfaceFormatKHR> formats = physDevice.getSurfaceFormatsKHR(surface);
    std::vector<vk::PresentModeKHR> presentModes = physDevice.getSurfacePresentModesKHR(surface);

    // Surface format (color depth)
    vk::SurfaceFormatKHR format = Swapchain::ChooseSurfaceFormat(formats);
    // Presentation mode (conditions for swapping images)
    vk::PresentModeKHR presentMode = Swapchain::ChoosePresentMode(presentModes);
    // Swap extent (resolution of images in the swap chain)
    vk::Extent2D extent = Swapchain::ChooseExtent(capabilities);

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
        RenderingContext::GetInstance().GetSurface(),
        imageCount,
        format.format,
        format.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment
    );


    const QueueFamilyIndices& indices = m_owner->GetQueueFamilyIndices();
    uint32_t queueFamilyIndices[] = { indices.graphics.value(), indices.present.value() };

    if (indices.graphics != indices.present) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    // Old swap chain is default
    swapchain.Create(swapchain, createInfo, *this,
        format.format, extent);
}

void Device::CreateRenderPass()
{
    // ATTACHMENTS
    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = swapchain.imageFormat;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;					// Number of samples to write for multisampling
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;				// Describes what to do with attachment before rendering
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;			// Describes what to do with attachment after rendering
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;	// Describes what to do with stencil before rendering
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;	// Describes what to do with stencil after rendering

    // Framebuffer data will be stored as an image, but images can be given different data layouts
    // to give optimal use for certain operations
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;			// Image data layout before render pass starts
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;		// Image data layout after render pass (to change to)


    // Depth
    vk::AttachmentDescription depthAttachment = {};
    depthAttachment.format = depthBuffer.GetFormat();
    depthAttachment.samples = vk::SampleCountFlagBits::e1;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
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

    renderPass.Create(createInfo, *this);
}

void Device::CreateGraphicsPipeline()
{
    // auto vertSrc = utils::ReadFile(std::string(ASSET_DIR) + "Shaders/vert.spv");
    // auto fragSrc = utils::ReadFile(std::string(ASSET_DIR) + "Shaders/frag.spv");
    //
    // vk::ShaderModuleCreateInfo shaderInfo;
    // shaderInfo.codeSize = vertSrc.size();
    // shaderInfo.pCode = reinterpret_cast<const uint32_t*>(vertSrc.data());
    // ShaderModule vertModule;
    // vertModule.Create(vertModule, shaderInfo, *this);
    //
    // shaderInfo.codeSize = fragSrc.size();
    // shaderInfo.pCode = reinterpret_cast<const uint32_t*>(fragSrc.data());
    // ShaderModule fragModule;
    // fragModule.Create(fragModule, shaderInfo, *this);
    //
    // static const char* entryName = "main";
    //
    // vk::PipelineShaderStageCreateInfo vertInfo(
    //     {},
    //     vk::ShaderStageFlagBits::eVertex,
    //     vertModule,
    //     entryName
    // );
    //
    // vk::PipelineShaderStageCreateInfo fragInfo(
    //     {},
    //     vk::ShaderStageFlagBits::eFragment,
    //     fragModule,
    //     entryName
    // );
    //
    // vk::PipelineShaderStageCreateInfo shaderStages[] = { vertInfo, fragInfo };
    //
    // // VERTEX BINDING DATA
    // vk::VertexInputBindingDescription bindDesc = {};
    // // Binding position (can bind multiple streams)
    // bindDesc.binding = 0;
    // bindDesc.stride = sizeof(Vertex);
    // // Instancing option, draw one object at a time in this case
    // bindDesc.inputRate = vk::VertexInputRate::eVertex;
    //
    // // VERTEX ATTRIB DATA
    // std::array<vk::VertexInputAttributeDescription, Vertex::NUM_ATTRIBS> attribDesc;
    //
    // // Position attribute
    // // Binding the data is at (should be same as above)
    // attribDesc[0].binding = 0;
    // // Location in shader where data is read from
    // attribDesc[0].location = 0;
    // // Format for the data being sent
    // attribDesc[0].format = vk::Format::eR32G32B32Sfloat;
    // // Where the attribute begins as an offset from the beginning
    // // of the structures
    // attribDesc[0].offset = offsetof(Vertex, pos);
    //
    // attribDesc[1].binding = 0;
    // attribDesc[1].location = 1;
    // attribDesc[1].format = vk::Format::eR32G32B32Sfloat;
    // attribDesc[1].offset = offsetof(Vertex, color);
    //
    //
    // // VERTEX INPUT
    // vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    // vertexInputInfo.vertexBindingDescriptionCount = 1;
    // vertexInputInfo.pVertexBindingDescriptions = &bindDesc;
    // // Data spacing / stride info
    // vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDesc.size());
    // vertexInputInfo.pVertexAttributeDescriptions = attribDesc.data();
    //
    // // INPUT ASSEMBLY
    // vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
    // inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    // inputAssembly.primitiveRestartEnable = VK_FALSE;
    //
    // // VIEWPORT
    //
    // vk::Extent2D extent = swapchain.GetExtent();
    //
    // vk::Viewport viewport{};
    // viewport.x = 0.0f;
    // viewport.y = 0.0f;
    // viewport.width = (float)extent.width;
    // viewport.height = (float)extent.height;
    // viewport.minDepth = 0.0f;
    // viewport.maxDepth = 1.0f;
    //
    // vk::Rect2D scissor;
    //
    // scissor.extent = extent;
    // scissor.offset = vk::Offset2D(0, 0);
    //
    // vk::PipelineViewportStateCreateInfo viewportState;
    // viewportState.viewportCount = 1;
    // viewportState.pViewports = &viewport;
    // viewportState.scissorCount = 1;
    // viewportState.pScissors = &scissor;
    //
    // // -- DYNAMIC STATES --
    // // Dynamic states to enable
    // //std::vector<VkDynamicState> dynamicStateEnables;
    // //dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);	// Dynamic Viewport : Can resize in command buffer with vkCmdSetViewport(commandbuffer, 0, 1, &viewport);
    // //dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);	// Dynamic Scissor	: Can resize in command buffer with vkCmdSetScissor(commandbuffer, 0, 1, &scissor);
    //
    // //// Dynamic State creation info
    // //VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    // //dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    // //dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
    // //dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();
    //
    // vk::PipelineRasterizationStateCreateInfo rasterizeState;
    // rasterizeState.depthClampEnable = VK_FALSE;			// Change if fragments beyond near/far planes are clipped (default) or clamped to plane
    // rasterizeState.rasterizerDiscardEnable = VK_FALSE;	// Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
    // rasterizeState.polygonMode = vk::PolygonMode::eFill;	// How to handle filling points between vertices
    // rasterizeState.lineWidth = 1.0f;						// How thick lines should be when drawn
    // rasterizeState.cullMode = vk::CullModeFlagBits::eBack;		// Which face of a triangle to cull
    // rasterizeState.frontFace = vk::FrontFace::eCounterClockwise;	// Winding to determine which side is front
    // rasterizeState.depthBiasEnable = VK_FALSE;			// Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)
    //
    // vk::PipelineMultisampleStateCreateInfo multisampleState;
    // // Whether or not to multi-sample
    // multisampleState.sampleShadingEnable = VK_FALSE;
    // // Number of samples per fragment
    // multisampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;
    //
    //
    // // Blending
    // vk::PipelineColorBlendAttachmentState colorBlendAttachments;
    // colorBlendAttachments.colorWriteMask = vk::ColorComponentFlags(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
    // colorBlendAttachments.blendEnable = VK_TRUE;
    //
    // // Blending uses equation: (srcColorBlendFactor * new colour) colorBlendOp (dstColorBlendFactor * old colour)
    // colorBlendAttachments.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    // colorBlendAttachments.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    // colorBlendAttachments.colorBlendOp = vk::BlendOp::eAdd;
    //
    // // Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new colour) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old colour)
    // //			   (new colour alpha * new colour) + ((1 - new colour alpha) * old colour)
    // colorBlendAttachments.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    // colorBlendAttachments.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    // colorBlendAttachments.alphaBlendOp = vk::BlendOp::eAdd;
    //
    //
    // vk::PipelineColorBlendStateCreateInfo colorBlendState;
    // colorBlendState.logicOpEnable = VK_FALSE;
    // colorBlendState.attachmentCount = 1;
    // colorBlendState.pAttachments = &colorBlendAttachments;
    //
    // vk::PipelineLayoutCreateInfo layout = {};
    // layout.setLayoutCount = 1;
    // layout.pSetLayouts = &descriptorSetLayout;
    // layout.pushConstantRangeCount = 1;
    // layout.pPushConstantRanges = &pushRange;
    //
    // pipelineLayout.Create(pipelineLayout, layout, *this);
    //
    // // Depth testing
    // vk::PipelineDepthStencilStateCreateInfo depthStencilState = {};
    // depthStencilState.depthTestEnable = VK_TRUE;
    // depthStencilState.depthWriteEnable = VK_TRUE;
    // depthStencilState.depthCompareOp = vk::CompareOp::eLess;
    // // if this was true, you can fill out min/max bounds in this createinfo
    // depthStencilState.depthBoundsTestEnable = VK_FALSE;
    // depthStencilState.stencilTestEnable = VK_FALSE;
    //
    //
    // vk::GraphicsPipelineCreateInfo pipelineInfo;
    // pipelineInfo.stageCount = 2;									// Number of shader stages
    // pipelineInfo.pStages = shaderStages;							// List of shader stages
    // pipelineInfo.pVertexInputState = &vertexInputInfo;		// All the fixed function pipeline states
    // pipelineInfo.pInputAssemblyState = &inputAssembly;
    // pipelineInfo.pViewportState = &viewportState;
    // pipelineInfo.pDynamicState = nullptr;
    // pipelineInfo.pRasterizationState = &rasterizeState;
    // pipelineInfo.pMultisampleState = &multisampleState;
    // pipelineInfo.pColorBlendState = &colorBlendState;
    // pipelineInfo.pDepthStencilState = &depthStencilState;
    // pipelineInfo.layout = (pipelineLayout);							// Pipeline Layout pipeline should use
    // pipelineInfo.renderPass = renderPass;							// Render pass description the pipeline is compatible with
    // pipelineInfo.subpass = 0;										// Subpass of render pass to use with pipeline
    //
    // graphicsPipeline.Create(graphicsPipeline, pipelineInfo, *this, vk::PipelineCache(), 1);
    //
    // if (bool(graphicsPipeline) != VK_TRUE)
    // {
    //     throw std::runtime_error("Failed to create graphics pipeline");
    // }
    //
    // vertModule.Destroy();
    // fragModule.Destroy();
}

void Device::CreateDepthBuffer(bool recreate)
{
    if (recreate) depthBuffer.Destroy();

    depthBuffer.Create(*this);
}

void Device::CreateFramebuffers(bool recreate)
{
    if (recreate)
    {
		for (auto& fb : framebuffers)
			fb.Destroy();
    }

    const auto& imageViews = swapchain.imageViews;
    const auto& extent = swapchain.extent;
    size_t imageViewsSize = imageViews.size();
    framebuffers.resize(imageViewsSize);

    vk::FramebufferCreateInfo createInfo;
    // FB width/height
    createInfo.width = extent.width;
    createInfo.height = extent.height;
    // FB layers
    createInfo.layers = 1;
    createInfo.renderPass = renderPass;
    createInfo.attachmentCount = 2;


    vk::ImageView depthView = depthBuffer.GetImageView();
    for (size_t i = 0; i < imageViewsSize; ++i)
    {
        std::array<vk::ImageView, 2> attachments = {
            imageViews[i].Get(),
            depthView
        };

        // List of attachments 1 to 1 with render pass
        createInfo.pAttachments = attachments.data();

        framebuffers[i].Create(&framebuffers[i], createInfo, *this);
    }
}

void Device::CreateCommandPool()
{
    QueueFamilyIndices indices = m_owner->GetQueueFamilyIndices();

    vk::CommandPoolCreateInfo poolInfo;
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolInfo.queueFamilyIndex = indices.graphics.value();

    commandPool.Create(commandPool, poolInfo, *this);
}

void Device::CreateCommandBuffers(bool recreate)
{
    if (recreate)
    {
		freeCommandBuffers(commandPool.Get(),
						   drawCmdBuffers.size(),
						   drawCmdBuffers.data());
    }
	
    size_t fbSize = framebuffers.size();
    drawCmdBuffers.resize(fbSize);

    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = commandPool.Get();
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = static_cast<uint32_t>(fbSize);

    utils::CheckVkResult(allocateCommandBuffers(&allocInfo, drawCmdBuffers.data()), "Failed to allocate command buffers");
}

void Device::RecordCommandBuffers(uint32_t imageIndex)
{
}

void Device::CreateSync()
{
    imageAvailable.resize(MAX_FRAME_DRAWS);
    renderFinished.resize(MAX_FRAME_DRAWS);
    drawFences.resize(MAX_FRAME_DRAWS);

    vk::SemaphoreCreateInfo semInfo{};
    vk::FenceCreateInfo fenceInfo{};
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
    {
        imageAvailable[i].Create(imageAvailable[i], semInfo, *this);
        renderFinished[i].Create(renderFinished[i], semInfo, *this);
        drawFences[i].Create(drawFences[i], fenceInfo, *this);
    }
}

void Device::DrawFrame(const uint32_t frameIndex)
{
    // // Freeze until previous image is drawn
    // waitForFences(1, &drawFences[frameIndex], VK_TRUE, std::numeric_limits<uint64_t>::max());
    // // Close the fence for this current frame again
    // resetFences(1, &drawFences[frameIndex]);
    //
    // acquireNextImageKHR(swapchain.Get(), std::numeric_limits<uint64_t>().max(), 
    //     imageAvailable[frameIndex].Get(), nullptr, &);
    //
    //
    // RecordCommandBuffers(imageIndex);
    // UpdateUniformBuffers(imageIndex);
    //
    // std::array<vk::PipelineStageFlags, 1> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    //
    // vk::SubmitInfo submitInfo;
    //
    // submitInfo.waitSemaphoreCount = 1;
    // // Semaphores to wait on 
    // submitInfo.pWaitSemaphores = &imageAvailable[frameIndex];
    // // Stages to check semaphores at
    // submitInfo.pWaitDstStageMask = waitStages.data();
    //
    // // Number of command buffers to submit
    // submitInfo.commandBufferCount = 1;
    // // Command buffers for submission
    // submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
    //
    // submitInfo.signalSemaphoreCount = 1;
    // // Signals command buffer is finished
    // submitInfo.pSignalSemaphores = &renderFinished[frameIndex];
    //
    //
    // utils::CheckVkResult(graphicsQueue.submit(1, &submitInfo, drawFences[frameIndex]), 
    //     "Failed to submit draw command buffer");
    //
    // vk::PresentInfoKHR presentInfo{};
    // presentInfo.waitSemaphoreCount = 1;
    // presentInfo.pWaitSemaphores = &renderFinished[frameIndex];
    //
    // std::array<vk::SwapchainKHR, 1> swapchains = { swapchain };
    // presentInfo.swapchainCount = (uint32_t)swapchains.size();
    // presentInfo.pSwapchains = swapchains.data();
    // presentInfo.pImageIndices = &imageIndex;
    //
    // utils::CheckVkResult(presentQueue.presentKHR(presentInfo), "Failed to present image");

}

void Device::CreateDescriptorSetLayout()
{
    // // UboViewProjection binding info
    // vk::DescriptorSetLayoutBinding vpBinding = {};
    // vpBinding.binding = 0;
    // vpBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    // vpBinding.descriptorCount = 1;
    // vpBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
    // // Sampler becomes immutable if set
    // vpBinding.pImmutableSamplers = nullptr;
    //
    // //vk::DescriptorSetLayoutBinding modelBinding = {};
    // //modelBinding.binding = 1;
    // //modelBinding.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
    // //modelBinding.descriptorCount = 1;
    // //modelBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
    // //modelBinding.pImmutableSamplers = nullptr;
    //
    // std::vector<vk::DescriptorSetLayoutBinding> bindings = { vpBinding };
    //
    // vk::DescriptorSetLayoutCreateInfo createInfo = {};
    // createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    // createInfo.pBindings = bindings.data();
    //
    // // Create descriptor set layout
    // DescriptorSetLayout::Create(descriptorSetLayout, createInfo, *this);
}

void Device::CreateUniformBuffers()
{
    // vk::DeviceSize vpBufferSize = sizeof(UboViewProjection);
    // //vk::DeviceSize modelBufferSize = modelUniformAlignment * MAX_OBJECTS;
    //
    // const auto& images = swapchain.GetImages();
    // // One uniform buffer for each image
    // //uniformBufferModel.resize(images.size());
    // uniformBufferViewProjection.resize(images.size());
    //
    // //vk::BufferCreateInfo modelCreateInfo = {};
    // //modelCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
    // //modelCreateInfo.size = modelBufferSize;
    //
    // vk::BufferCreateInfo vpCreateInfo = {};
    // vpCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
    // vpCreateInfo.size = vpBufferSize;
    //
    // VmaAllocationCreateInfo aCreateInfo = {};
    // aCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    //
    // for (size_t i = 0; i < images.size(); ++i)
    // {
    //     //uniformBufferModel[i].Create(modelCreateInfo, aCreateInfo, *this, allocator);
    //     uniformBufferViewProjection[i].Create(vpCreateInfo, aCreateInfo, *this);
    // }
}

void Device::CreateDescriptorPool()
{
}

void Device::CreateDescriptorSets()
{
 
}

void Device::AllocateDynamicBufferTransferSpace()
{
    //// Allocate memory for objects
    //modelTransferSpace = (Model*)_aligned_malloc(
    //    modelUniformAlignment * MAX_OBJECTS, 
    //    modelUniformAlignment
    //);

}

void Device::CreatePushConstantRange()
{
}



void Device::UpdateUniformBuffers(uint32_t imageIndex)
{


    // Copy model data
    //size_t numObjects = objects.size();
    //for (size_t i = 0; i < numObjects; ++i)
    //{
    //    Model* model = (Model*)((uint64_t) modelTransferSpace + (i * modelUniformAlignment));
    //    *model = objects[i].GetModel();
    //}

    //auto& modelBuffer = uniformBufferModel[imageIndex];
    //auto& modelAllocInfo = modelBuffer.GetAllocationInfo();
    //memory = modelAllocInfo.deviceMemory;
    //offset = modelAllocInfo.offset;
    //mapMemory(memory, offset, modelUniformAlignment * numObjects, {}, &data);
    //memcpy(data, modelTransferSpace, modelUniformAlignment * numObjects);
    //unmapMemory(memory);
}

void Device::Update(float dt)
{
   }

void Device::PrepareFrame(const uint32_t frameIndex)
{
    // Freeze until previous image is drawn
    waitForFences(1, &drawFences[frameIndex], VK_TRUE, std::numeric_limits<uint64_t>::max());
    // Close the fence for this current frame again
    resetFences(1, &drawFences[frameIndex]);
    auto result = acquireNextImageKHR(swapchain.Get(), std::numeric_limits<uint64_t>::max(), imageAvailable[frameIndex].Get(),
                                      nullptr, &imageIndex);
    // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
    if ((result == vk::Result::eErrorOutOfDateKHR) || (result == vk::Result::eSuboptimalKHR)) {
        // windowResize();
    }
    else {
        utils::CheckVkResult(result, "Failed to prepare render frame");
    }

	
}

void Device::SubmitFrame(const uint32_t frameIndex)
{
    vk::PresentInfoKHR presentInfo{};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pWaitSemaphores = &renderFinished[frameIndex];
    presentInfo.waitSemaphoreCount = 1;
    vk::Result result = graphicsQueue.presentKHR(presentInfo);

    if (!((result == vk::Result::eSuccess) || (result == vk::Result::eSuboptimalKHR))) {
        if (result == vk::Result::eErrorOutOfDateKHR) {
            // Swap chain is no longer compatible with the surface and needs to be recreated
            // windowResize();
            return;
        }
        else {
            utils::CheckVkResult(result, "Failed to submit frame");
        }
    }
}
