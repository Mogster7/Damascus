//------------------------------------------------------------------------------
//
// File Name:	Device.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/18/2020
//
//------------------------------------------------------------------------------
#include "Renderer.h"
#include "Window.h"
#include <glm/gtc/matrix_transform.hpp>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

const std::vector<Vertex> Device::meshVerts = {
    { { -0.4, 0.4, 0.0 },{ 1.0f, 0.0f, 0.0f } },	// 0
    { { -0.4, -0.4, 0.0 },{ 1.0f, 0.0f, 0.0f } },	    // 1
    { { 0.4, -0.4, 0.0 },{ 1.0f, 0.0f, 0.0f } },    // 2
    { { 0.4, 0.4, 0.0 },{ 1.0f, 0.0f, 0.0f } },   // 3

};

const std::vector<Vertex> Device::meshVerts2 = {
    { { -0.25, 0.6, 0.0 },{ 0.0f, 0.0f, 1.0f } },	// 0
    { { -0.25, -0.6, 0.0 },{ 0.0f, 0.0f, 1.0f } },	    // 1
    { { 0.25, -0.6, 0.0 },{ 0.0f, 0.0f, 1.0f } },    // 2
    { { 0.25, 0.6, 0.0 },{ 0.0f, 0.0f, 1.0f } },   // 3

};

const std::vector<uint32_t> Device::meshIndices = {
    0, 1, 2,
    2, 3, 0
};


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
    depthBuffer.Create(*this);
    CreateRenderPass();
    CreateDescriptorSetLayout();
    CreatePushConstantRange();
    CreateGraphicsPipeline();
    CreateFramebuffers();
    CreateCommandPool();

    const auto& extent = swapchain.GetExtent();
    uboViewProjection.projection = glm::perspective(glm::radians(45.0f), (float)extent.width / extent.height, 0.1f, 100.0f);
    uboViewProjection.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Invert up direction so that pos y is up 
    // its down by default in Vulkan
    uboViewProjection.projection[1][1] *= -1;

    objects.emplace_back().Create(*this, Device::meshVerts, Device::meshIndices);
    objects.emplace_back().Create(*this, Device::meshVerts2, Device::meshIndices);

    objects[0].SetModel(glm::translate(objects[0].GetModel(), glm::vec3(0.0f, 0.0f, -3.0f)));
    objects[1].SetModel(glm::translate(objects[1].GetModel(), glm::vec3(0.0f, 0.0f, -2.5f)));

    CreateUniformBuffers();
    AllocateDynamicBufferTransferSpace();
    CreateDescriptorPool();
    CreateDescriptorSets();

    CreateCommandBuffers();
    CreateSync();
}

void Device::CreateAllocator()
{
    ASSERT(allocator == nullptr, "Creating an existing member");
    VmaAllocatorCreateInfo info = {};
    info.instance = Renderer::GetInstance();
    info.physicalDevice = Renderer::GetPhysicalDevice();
    info.device = *this;

    vmaCreateAllocator(&info, &allocator);
}

void Device::Destroy()
{
    descriptorPool.Destroy();
    descriptorSetLayout.Destroy();

    for (size_t i = 0; i < swapchain.GetImages().size(); ++i)
    {
        uniformBufferViewProjection[i].Destroy();
    }

    for (size_t i = 0; i < objects.size(); ++i)
        objects[i].Destroy();

    for(auto& fence : drawFences)
        fence.Destroy();

    for (size_t i = 0; i < renderFinished.size(); ++i)
    {
        renderFinished[i].Destroy();
        imageAvailable[i].Destroy();
    }

    graphicsCmdPool.Destroy();
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

void Device::CreateSwapchain()
{    
    vk::PhysicalDevice physDevice = *m_owner;
    vk::SurfaceKHR surface = Renderer::GetInstance().GetSurface();

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
        Renderer::GetInstance().GetSurface(),
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
    colorAttachment.format = swapchain.GetImageFormat();
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
    auto vertSrc = utils::ReadFile("Shaders/vert.spv");
    auto fragSrc = utils::ReadFile("Shaders/frag.spv");

    vk::ShaderModuleCreateInfo shaderInfo;
    shaderInfo.codeSize = vertSrc.size();
    shaderInfo.pCode = reinterpret_cast<const uint32_t*>(vertSrc.data());
    ShaderModule vertModule;
    vertModule.Create(vertModule, shaderInfo, *this);

    shaderInfo.codeSize = fragSrc.size();
    shaderInfo.pCode = reinterpret_cast<const uint32_t*>(fragSrc.data());
    ShaderModule fragModule;
    fragModule.Create(fragModule, shaderInfo, *this);

    static const char* entryName = "main";

    vk::PipelineShaderStageCreateInfo vertInfo(
        {},
        vk::ShaderStageFlagBits::eVertex,
        vertModule,
        entryName
    );

    vk::PipelineShaderStageCreateInfo fragInfo(
        {},
        vk::ShaderStageFlagBits::eFragment,
        fragModule,
        entryName
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

    // Position attribute
    // Binding the data is at (should be same as above)
    attribDesc[0].binding = 0;
    // Location in shader where data is read from
    attribDesc[0].location = 0;
    // Format for the data being sent
    attribDesc[0].format = vk::Format::eR32G32B32Sfloat;
    // Where the attribute begins as an offset from the beginning
    // of the structures
    attribDesc[0].offset = offsetof(Vertex, pos);

    attribDesc[1].binding = 0;
    attribDesc[1].location = 1;
    attribDesc[1].format = vk::Format::eR32G32B32Sfloat;
    attribDesc[1].offset = offsetof(Vertex, color);


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

    vk::Extent2D extent = swapchain.GetExtent();

    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor;

    scissor.extent = extent;
    scissor.offset = vk::Offset2D(0, 0);

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // -- DYNAMIC STATES --
    // Dynamic states to enable
    //std::vector<VkDynamicState> dynamicStateEnables;
    //dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);	// Dynamic Viewport : Can resize in command buffer with vkCmdSetViewport(commandbuffer, 0, 1, &viewport);
    //dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);	// Dynamic Scissor	: Can resize in command buffer with vkCmdSetScissor(commandbuffer, 0, 1, &scissor);

    //// Dynamic State creation info
    //VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    //dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    //dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
    //dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();

    vk::PipelineRasterizationStateCreateInfo rasterizeState;
    rasterizeState.depthClampEnable = VK_FALSE;			// Change if fragments beyond near/far planes are clipped (default) or clamped to plane
    rasterizeState.rasterizerDiscardEnable = VK_FALSE;	// Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
    rasterizeState.polygonMode = vk::PolygonMode::eFill;	// How to handle filling points between vertices
    rasterizeState.lineWidth = 1.0f;						// How thick lines should be when drawn
    rasterizeState.cullMode = vk::CullModeFlagBits::eBack;		// Which face of a triangle to cull
    rasterizeState.frontFace = vk::FrontFace::eCounterClockwise;	// Winding to determine which side is front
    rasterizeState.depthBiasEnable = VK_FALSE;			// Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)

    vk::PipelineMultisampleStateCreateInfo multisampleState;
    // Whether or not to multi-sample
    multisampleState.sampleShadingEnable = VK_FALSE;
    // Number of samples per fragment
    multisampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;


    // Blending
    vk::PipelineColorBlendAttachmentState colorBlendAttachments;
    colorBlendAttachments.colorWriteMask = vk::ColorComponentFlags(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);
    colorBlendAttachments.blendEnable = VK_TRUE;

    // Blending uses equation: (srcColorBlendFactor * new colour) colorBlendOp (dstColorBlendFactor * old colour)
    colorBlendAttachments.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    colorBlendAttachments.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    colorBlendAttachments.colorBlendOp = vk::BlendOp::eAdd;

    // Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new colour) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old colour)
    //			   (new colour alpha * new colour) + ((1 - new colour alpha) * old colour)
    colorBlendAttachments.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachments.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachments.alphaBlendOp = vk::BlendOp::eAdd;


    vk::PipelineColorBlendStateCreateInfo colorBlendState;
    colorBlendState.logicOpEnable = VK_FALSE;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &colorBlendAttachments;

    vk::PipelineLayoutCreateInfo layout = {};
    layout.setLayoutCount = 1;
    layout.pSetLayouts = &descriptorSetLayout;
    layout.pushConstantRangeCount = 1;
    layout.pPushConstantRanges = &pushRange;

    pipelineLayout.Create(pipelineLayout, layout, *this);

    // Depth testing
    vk::PipelineDepthStencilStateCreateInfo depthStencilState = {};
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = vk::CompareOp::eLess;
    // if this was true, you can fill out min/max bounds in this createinfo
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.stencilTestEnable = VK_FALSE;


    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.stageCount = 2;									// Number of shader stages
    pipelineInfo.pStages = shaderStages;							// List of shader stages
    pipelineInfo.pVertexInputState = &vertexInputInfo;		// All the fixed function pipeline states
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.pRasterizationState = &rasterizeState;
    pipelineInfo.pMultisampleState = &multisampleState;
    pipelineInfo.pColorBlendState = &colorBlendState;
    pipelineInfo.pDepthStencilState = &depthStencilState;
    pipelineInfo.layout = (pipelineLayout);							// Pipeline Layout pipeline should use
    pipelineInfo.renderPass = renderPass;							// Render pass description the pipeline is compatible with
    pipelineInfo.subpass = 0;										// Subpass of render pass to use with pipeline

    graphicsPipeline.Create(graphicsPipeline, pipelineInfo, *this, vk::PipelineCache(), 1);

    if (bool(graphicsPipeline) != VK_TRUE)
    {
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    vertModule.Destroy();
    fragModule.Destroy();
}

void Device::CreateFramebuffers()
{
    const auto& imageViews = swapchain.GetImageViews();
    const auto& extent = swapchain.GetExtent();
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
            imageViews[i],
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

    graphicsCmdPool.Create(graphicsCmdPool, poolInfo, *this);
}

void Device::CreateCommandBuffers()
{
    size_t fbSize = framebuffers.size();
    commandBuffers.resize(fbSize);

    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = graphicsCmdPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = static_cast<uint32_t>(fbSize);

    utils::CheckVkResult(allocateCommandBuffers(&allocInfo, commandBuffers.data()), "Failed to allocate command buffers");
}

void Device::RecordCommandBuffers(uint32_t imageIndex)
{
    std::array<vk::ClearValue, 2> clearValues = {};
    clearValues[0].color = vk::ClearColorValue(std::array<float, 4>{0.6f, 0.65f, 0.4f, 1.0f });
    clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f);


    vk::RenderPassBeginInfo renderPassInfo;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.renderArea.extent = swapchain.GetExtent();
    renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    // Record command buffers
    vk::CommandBufferBeginInfo info(
        {},
        nullptr
    );

    auto& cmdBuf = commandBuffers[imageIndex];

    utils::CheckVkResult(cmdBuf.begin(&info), "Failed to begin recording command buffer");
    renderPassInfo.framebuffer = framebuffers[imageIndex];

    cmdBuf.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

    for (int j = 0; j < objects.size(); ++j)
    {
        const auto& mesh = objects[j];
        // Buffers to bind
        vk::Buffer vertexBuffers[] = { mesh.GetVertexBuffer() };
        // Offsets
        vk::DeviceSize offsets[] = { 0 };

        cmdBuf.bindVertexBuffers(0, 1, vertexBuffers, offsets);
        cmdBuf.bindIndexBuffer(mesh.GetIndexBuffer(), 0, vk::IndexType::eUint32);

        // Dynamic offset amount
        //uint32_t dynamicOffset = static_cast<uint32_t>(modelUniformAlignment) * j;

        cmdBuf.pushConstants(
            pipelineLayout, 
            // Stage
            vk::ShaderStageFlagBits::eVertex, 
            // Offset
            0, 
            // Size of data being pushed
            sizeof(Model), 
            // Actual data being pushed
            &objects[j].GetModel()
        );

        //// Bind descriptor sets
        cmdBuf.bindDescriptorSets(
            // Point of pipeline and layout
            vk::PipelineBindPoint::eGraphics,
            pipelineLayout,

            // First set, num sets, pointer to set
            0,
            1,
            &descriptorSets[imageIndex], // 1 to 1 with command buffers

            // Dynamic offsets
            //1,
            //&dynamicOffset
            0,
            nullptr
        );

        // Execute pipeline
        cmdBuf.drawIndexed(mesh.GetIndexCount(), 1, 0, 0, 0);
    }

    cmdBuf.endRenderPass();
    cmdBuf.end();
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
    uint32_t imageIndex;

    // Freeze until previous image is drawn
    waitForFences(1, &drawFences[frameIndex], VK_TRUE, std::numeric_limits<uint64_t>().max());
    // Close the fence for this current frame again
    resetFences(1, &drawFences[frameIndex]);

    acquireNextImageKHR(swapchain, std::numeric_limits<uint64_t>().max(), 
        imageAvailable[frameIndex], nullptr, &imageIndex);


    RecordCommandBuffers(imageIndex);
    UpdateUniformBuffers(imageIndex);

    std::array<vk::PipelineStageFlags, 1> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

    vk::SubmitInfo submitInfo;

    submitInfo.waitSemaphoreCount = 1;
    // Semaphores to wait on 
    submitInfo.pWaitSemaphores = &imageAvailable[frameIndex];
    // Stages to check semaphores at
    submitInfo.pWaitDstStageMask = waitStages.data();

    // Number of command buffers to submit
    submitInfo.commandBufferCount = 1;
    // Command buffers for submission
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

    submitInfo.signalSemaphoreCount = 1;
    // Signals command buffer is finished
    submitInfo.pSignalSemaphores = &renderFinished[frameIndex];


    utils::CheckVkResult(graphicsQueue.submit(1, &submitInfo, drawFences[frameIndex]), 
        "Failed to submit draw command buffer");

    vk::PresentInfoKHR presentInfo{};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinished[frameIndex];

    std::array<vk::SwapchainKHR, 1> swapchains = { swapchain };
    presentInfo.swapchainCount = (uint32_t)swapchains.size();
    presentInfo.pSwapchains = swapchains.data();
    presentInfo.pImageIndices = &imageIndex;

    utils::CheckVkResult(presentQueue.presentKHR(presentInfo), "Failed to present image");

}

void Device::CreateDescriptorSetLayout()
{
    // UboViewProjection binding info
    vk::DescriptorSetLayoutBinding vpBinding = {};
    vpBinding.binding = 0;
    vpBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    vpBinding.descriptorCount = 1;
    vpBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
    // Sampler becomes immutable if set
    vpBinding.pImmutableSamplers = nullptr;

    //vk::DescriptorSetLayoutBinding modelBinding = {};
    //modelBinding.binding = 1;
    //modelBinding.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
    //modelBinding.descriptorCount = 1;
    //modelBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
    //modelBinding.pImmutableSamplers = nullptr;

    std::vector<vk::DescriptorSetLayoutBinding> bindings = { vpBinding };

    vk::DescriptorSetLayoutCreateInfo createInfo = {};
    createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    createInfo.pBindings = bindings.data();

    // Create descriptor set layout
    DescriptorSetLayout::Create(descriptorSetLayout, createInfo, *this);
}

void Device::CreateUniformBuffers()
{
    vk::DeviceSize vpBufferSize = sizeof(UboViewProjection);
    //vk::DeviceSize modelBufferSize = modelUniformAlignment * MAX_OBJECTS;

    const auto& images = swapchain.GetImages();
    // One uniform buffer for each image
    //uniformBufferModel.resize(images.size());
    uniformBufferViewProjection.resize(images.size());

    //vk::BufferCreateInfo modelCreateInfo = {};
    //modelCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
    //modelCreateInfo.size = modelBufferSize;

    vk::BufferCreateInfo vpCreateInfo = {};
    vpCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
    vpCreateInfo.size = vpBufferSize;

    VmaAllocationCreateInfo aCreateInfo = {};
    aCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    for (size_t i = 0; i < images.size(); ++i)
    {
        //uniformBufferModel[i].Create(modelCreateInfo, aCreateInfo, *this, allocator);
        uniformBufferViewProjection[i].Create(vpCreateInfo, aCreateInfo, *this, allocator);
    }
}

void Device::CreateDescriptorPool()
{
    // How many descriptors, not descriptor sets
    vk::DescriptorPoolSize vpPoolSize = {};
    vpPoolSize.type = vk::DescriptorType::eUniformBuffer;
    vpPoolSize.descriptorCount = static_cast<uint32_t>(uniformBufferViewProjection.size());

    //vk::DescriptorPoolSize modelPoolSize = {};
    //modelPoolSize.type = vk::DescriptorType::eUniformBufferDynamic;
    //modelPoolSize.descriptorCount = static_cast<uint32_t>(uniformBufferModel.size());

    std::vector<vk::DescriptorPoolSize> poolSizes = { vpPoolSize };

    // Pool creation
    vk::DescriptorPoolCreateInfo poolCreateInfo = {};
    // Maximum number of descriptor sets that can be created from the pool
    poolCreateInfo.maxSets = static_cast<uint32_t>(swapchain.GetImages().size());
    // Number of pool sizes being passed 
    poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolCreateInfo.pPoolSizes = poolSizes.data();

    // Create descriptor pool
    descriptorPool.Create(descriptorPool, poolCreateInfo, *this);
}

void Device::CreateDescriptorSets()
{
    size_t ubSize = swapchain.GetImages().size();
    // One descriptor set for every uniform buffer
    descriptorSets.resize(ubSize);

    // Create one copy of our descriptor set layout per buffer
    std::vector<vk::DescriptorSetLayout> setLayouts(ubSize, descriptorSetLayout.Get());

    vk::DescriptorSetAllocateInfo allocInfo = {};
    allocInfo.descriptorPool = descriptorPool;
    // Number of sets to allocate
    allocInfo.descriptorSetCount = static_cast<uint32_t>(ubSize);
    // Layouts to use to allocate sets (1:1 relationship)
    allocInfo.pSetLayouts = setLayouts.data();

    utils::CheckVkResult(allocateDescriptorSets(&allocInfo, descriptorSets.data()), 
        "Failed to allocate descriptor sets");


    // CONNECT DESCRIPTOR SET TO BUFFER
    // UPDATE ALL DESCRIPTOR SET BINDINGS
    vk::WriteDescriptorSet vpSetWrite = {};
    vk::DescriptorBufferInfo vpBufferInfo = {};

    // BUFFER INFO
    vpBufferInfo.offset = 0;
    vpBufferInfo.range = sizeof(UboViewProjection);

    // SET WRITING INFO
    // Matches with shader layout binding
    vpSetWrite.dstBinding = 0;
    // Index of array to update
    vpSetWrite.dstArrayElement = 0;
    vpSetWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    // Amount to update
    vpSetWrite.descriptorCount = 1;

    //// Model version
    //vk::DescriptorBufferInfo modelBufferInfo = {};
    //modelBufferInfo.offset = 0;
    //modelBufferInfo.range = modelUniformAlignment;

    //vk::WriteDescriptorSet modelSetWrite = {};
    //modelSetWrite.dstBinding = 1;
    //modelSetWrite.dstArrayElement = 0;
    //modelSetWrite.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
    //modelSetWrite.descriptorCount = 1;

    // Update all descriptor set buffer bindings
    for (size_t i = 0; i < ubSize; ++i)
    {
        // Buffer to get data from
        vpBufferInfo.buffer = uniformBufferViewProjection[i];
        //modelBufferInfo.buffer = uniformBufferModel[i];

        // Current descriptor set 
        vpSetWrite.dstSet = descriptorSets[i];
        //modelSetWrite.dstSet = descriptorSets[i];

        // Update with new buffer info
        vpSetWrite.pBufferInfo = &vpBufferInfo;
        //modelSetWrite.pBufferInfo = &modelBufferInfo;

        std::array<vk::WriteDescriptorSet, 1> setWrites = { vpSetWrite };

        updateDescriptorSets(
            static_cast<uint32_t>(setWrites.size()), 
            setWrites.data(), 
            0, 
            nullptr);
    }

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
    // NO create needed

    // Where push constant is located
    pushRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
    pushRange.offset = 0;
    pushRange.size = sizeof(Model);
}



void Device::UpdateUniformBuffers(uint32_t imageIndex)
{
    // Copy view & projection data
    auto& vpBuffer = uniformBufferViewProjection[imageIndex];
    void* data;
    const auto& vpAllocInfo = vpBuffer.GetAllocationInfo();
    vk::DeviceMemory memory = vpAllocInfo.deviceMemory;
    vk::DeviceSize offset = vpAllocInfo.offset;
    mapMemory(memory, offset, sizeof(UboViewProjection), {}, &data);
    memcpy(data, &uboViewProjection, sizeof(UboViewProjection));
    unmapMemory(memory);

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
    static float speed = 90.0f;
    static float speed2 = 230.0f;

    const auto& model = objects[0].GetModel();
    objects[0].SetModel(glm::rotate(model, glm::radians(speed * dt), { 0.0f, 0.0f,1.0f }));

    const auto& model2 = objects[1].GetModel();
    objects[1].SetModel(glm::rotate(model2, glm::radians(speed2 * dt), { 0.0f, 1.0f,1.0f }));
}
