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

const eastl::vector<Vertex> Device::meshVerts = {
    { { -0.4, 0.4, 0.0 },{ 1.0f, 0.0f, 0.0f } },	// 0
    { { -0.4, -0.4, 0.0 },{ 1.0f, 0.0f, 0.0f } },	    // 1
    { { 0.4, -0.4, 0.0 },{ 1.0f, 0.0f, 0.0f } },    // 2
    { { 0.4, 0.4, 0.0 },{ 1.0f, 0.0f, 0.0f } },   // 3

};

const eastl::vector<Vertex> Device::meshVerts2 = {
    { { -0.25, 0.6, 0.0 },{ 0.0f, 0.0f, 1.0f } },	// 0
    { { -0.25, -0.6, 0.0 },{ 0.0f, 0.0f, 1.0f } },	    // 1
    { { 0.25, -0.6, 0.0 },{ 0.0f, 0.0f, 1.0f } },    // 2
    { { 0.25, 0.6, 0.0 },{ 0.0f, 0.0f, 1.0f } },   // 3

};

const eastl::vector<uint32_t> Device::meshIndices = {
    0, 1, 2,
    2, 3, 0
};


void Device::Initialization()
{
    const QueueFamilyIndices& indices = m_Owner->GetQueueFamilyIndices();
    getQueue(indices.graphics.value(), 0, &m_GraphicsQueue);
    getQueue(indices.present.value(), 0, &m_PresentQueue);
    //m_MinUniformBufferOffset = m_Owner->GetMinimumUniformBufferOffset();
    //// Calculate alignment of model uniform 
    //m_ModelUniformAlignment = (sizeof(Model) + m_MinUniformBufferOffset - 1)
    //                            & ~(m_MinUniformBufferOffset - 1);

    CreateAllocator();
    CreateSwapchain();
    m_DepthBuffer.Create(*this);
    CreateRenderPass();
    CreateDescriptorSetLayout();
    CreatePushConstantRange();
    CreateGraphicsPipeline();
    CreateFramebuffers();
    CreateCommandPool();

    const auto& extent = m_Swapchain.GetExtent();
    m_UboViewProjection.projection = glm::perspective(glm::radians(45.0f), (float)extent.width / extent.height, 0.1f, 100.0f);
    m_UboViewProjection.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Invert up direction so that pos y is up 
    // its down by default in Vulkan
    m_UboViewProjection.projection[1][1] *= -1;

    objects.emplace_back().Create(*this, Device::meshVerts, Device::meshIndices);
    objects.emplace_back().Create(*this, Device::meshVerts2, Device::meshIndices);

    objects[0].SetModel(glm::translate(objects[0].GetModel().model, glm::vec3(0.0f, 0.0f, -3.0f)));
    objects[1].SetModel(glm::translate(objects[1].GetModel().model, glm::vec3(0.0f, 0.0f, -2.5f)));

    CreateUniformBuffers();
    AllocateDynamicBufferTransferSpace();
    CreateDescriptorPool();
    CreateDescriptorSets();

    CreateCommandBuffers();
    CreateSync();
}

void Device::CreateAllocator()
{
    ASSERT(m_Allocator == nullptr, "Creating an existing member");
    VmaAllocatorCreateInfo info = {};
    info.instance = Renderer::GetInstance();
    info.physicalDevice = Renderer::GetPhysicalDevice();
    info.device = *this;

    vmaCreateAllocator(&info, &m_Allocator);
}

void Device::Destroy()
{
    m_DescriptorPool.Destroy();
    m_DescriptorSetLayout.Destroy();

    for (size_t i = 0; i < m_Swapchain.GetImages().size(); ++i)
    {
        m_UniformBufferViewProjection[i].Destroy();
    }

    for (size_t i = 0; i < objects.size(); ++i)
        objects[i].Destroy();

    for(auto& fence : m_DrawFences)
        fence.Destroy();

    for (size_t i = 0; i < m_RenderFinished.size(); ++i)
    {
        m_RenderFinished[i].Destroy();
        m_ImageAvailable[i].Destroy();
    }

    m_GraphicsCmdPool.Destroy();
    for (auto& fb : m_Framebuffers)
        fb.Destroy();

    m_GraphicsPipeline.Destroy();
    m_PipelineLayout.Destroy();

    m_RenderPass.Destroy();
    m_DepthBuffer.Destroy();
    m_Swapchain.Destroy();
    vmaDestroyAllocator(m_Allocator);

    destroy();
}

void Device::CreateSwapchain()
{    
    vk::PhysicalDevice physDevice = *m_Owner;
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


    const QueueFamilyIndices& indices = m_Owner->GetQueueFamilyIndices();
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
    m_Swapchain.Create(m_Swapchain, createInfo, *this,
        format.format, extent);
}

void Device::CreateRenderPass()
{
    // ATTACHMENTS
    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = m_Swapchain.GetImageFormat();
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
    depthAttachment.format = m_DepthBuffer.GetFormat();
    depthAttachment.samples = vk::SampleCountFlagBits::e1;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
    depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;


    eastl::array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

    // REFERENCES
    // Attachment reference uses an attachment index that refers to index in the attachment list passed to m_RenderPassCreateInfo
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

    m_RenderPass.Create(createInfo, *this);
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

    vk::Extent2D extent = m_Swapchain.GetExtent();

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
    layout.pSetLayouts = &m_DescriptorSetLayout;
    layout.pushConstantRangeCount = 1;
    layout.pPushConstantRanges = &m_PushRange;

    m_PipelineLayout.Create(m_PipelineLayout, layout, *this);

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
    pipelineInfo.layout = m_PipelineLayout;							// Pipeline Layout pipeline should use
    pipelineInfo.renderPass = m_RenderPass;							// Render pass description the pipeline is compatible with
    pipelineInfo.subpass = 0;										// Subpass of render pass to use with pipeline

    m_GraphicsPipeline.Create(m_GraphicsPipeline, pipelineInfo, *this, vk::PipelineCache(), 1);

    if (bool(m_GraphicsPipeline) != VK_TRUE)
    {
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    vertModule.Destroy();
    fragModule.Destroy();
}

void Device::CreateFramebuffers()
{
    const auto& imageViews = m_Swapchain.GetImageViews();
    const auto& extent = m_Swapchain.GetExtent();
    size_t imageViewsSize = imageViews.size();
    m_Framebuffers.resize(imageViewsSize);

    vk::FramebufferCreateInfo createInfo;
    // FB width/height
    createInfo.width = extent.width;
    createInfo.height = extent.height;
    // FB layers
    createInfo.layers = 1;
    createInfo.renderPass = m_RenderPass;
    createInfo.attachmentCount = 2;


    vk::ImageView depthView = m_DepthBuffer.GetImageView();
    for (size_t i = 0; i < imageViewsSize; ++i)
    {
        std::array<vk::ImageView, 2> attachments = {
            imageViews[i],
            depthView
        };

        // List of attachments 1 to 1 with render pass
        createInfo.pAttachments = attachments.data();

        m_Framebuffers[i].Create(&m_Framebuffers[i], createInfo, *this);
    }
}

void Device::CreateCommandPool()
{
    QueueFamilyIndices indices = m_Owner->GetQueueFamilyIndices();

    vk::CommandPoolCreateInfo poolInfo;
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolInfo.queueFamilyIndex = indices.graphics.value();

    m_GraphicsCmdPool.Create(m_GraphicsCmdPool, poolInfo, *this);
}

void Device::CreateCommandBuffers()
{
    size_t fbSize = m_Framebuffers.size();
    m_CommandBuffers.resize(fbSize);

    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = m_GraphicsCmdPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = static_cast<uint32_t>(fbSize);

    utils::CheckVkResult(allocateCommandBuffers(&allocInfo, m_CommandBuffers.data()), "Failed to allocate command buffers");
}

void Device::RecordCommandBuffers(uint32_t imageIndex)
{
    eastl::array<vk::ClearValue, 2> clearValues = {};
    clearValues[0].color = vk::ClearColorValue(std::array<float, 4>{0.6f, 0.65f, 0.4f, 1.0f });
    clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f);


    vk::RenderPassBeginInfo m_RenderPassInfo;
    m_RenderPassInfo.renderPass = m_RenderPass;
    m_RenderPassInfo.renderArea.extent = m_Swapchain.GetExtent();
    m_RenderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
    m_RenderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    m_RenderPassInfo.pClearValues = clearValues.data();

    // Record command buffers
    vk::CommandBufferBeginInfo info(
        {},
        nullptr
    );

    auto& cmdBuf = m_CommandBuffers[imageIndex];

    utils::CheckVkResult(cmdBuf.begin(&info), "Failed to begin recording command buffer");
    m_RenderPassInfo.framebuffer = m_Framebuffers[imageIndex];

    cmdBuf.beginRenderPass(&m_RenderPassInfo, vk::SubpassContents::eInline);
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, m_GraphicsPipeline);

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
        //uint32_t dynamicOffset = static_cast<uint32_t>(m_ModelUniformAlignment) * j;

        cmdBuf.pushConstants(
            m_PipelineLayout, 
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
            m_PipelineLayout,

            // First set, num sets, pointer to set
            0,
            1,
            &m_DescriptorSets[imageIndex], // 1 to 1 with command buffers

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
    m_ImageAvailable.resize(MAX_FRAME_DRAWS);
    m_RenderFinished.resize(MAX_FRAME_DRAWS);
    m_DrawFences.resize(MAX_FRAME_DRAWS);

    vk::SemaphoreCreateInfo semInfo{};
    vk::FenceCreateInfo fenceInfo{};
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
    {
        m_ImageAvailable[i].Create(m_ImageAvailable[i], semInfo, *this);
        m_RenderFinished[i].Create(m_RenderFinished[i], semInfo, *this);
        m_DrawFences[i].Create(m_DrawFences[i], fenceInfo, *this);
    }
}

void Device::DrawFrame(const uint32_t frameIndex)
{
    uint32_t imageIndex;

    // Freeze until previous image is drawn
    waitForFences(1, &m_DrawFences[frameIndex], VK_TRUE, std::numeric_limits<uint64_t>().max());
    // Close the fence for this current frame again
    resetFences(1, &m_DrawFences[frameIndex]);

    acquireNextImageKHR(m_Swapchain, std::numeric_limits<uint64_t>().max(), 
        m_ImageAvailable[frameIndex], nullptr, &imageIndex);


    RecordCommandBuffers(imageIndex);
    UpdateUniformBuffers(imageIndex);

    std::array<vk::PipelineStageFlags, 1> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

    vk::SubmitInfo submitInfo;

    submitInfo.waitSemaphoreCount = 1;
    // Semaphores to wait on 
    submitInfo.pWaitSemaphores = &m_ImageAvailable[frameIndex];
    // Stages to check semaphores at
    submitInfo.pWaitDstStageMask = waitStages.data();

    // Number of command buffers to submit
    submitInfo.commandBufferCount = 1;
    // Command buffers for submission
    submitInfo.pCommandBuffers = &m_CommandBuffers[imageIndex];

    submitInfo.signalSemaphoreCount = 1;
    // Signals command buffer is finished
    submitInfo.pSignalSemaphores = &m_RenderFinished[frameIndex];


    utils::CheckVkResult(m_GraphicsQueue.submit(1, &submitInfo, m_DrawFences[frameIndex]), "Failed to submit draw command buffer");

    vk::PresentInfoKHR presentInfo{};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_RenderFinished[frameIndex];

    std::array<vk::SwapchainKHR, 1> m_Swapchains = { m_Swapchain };
    presentInfo.swapchainCount = (uint32_t)m_Swapchains.size();
    presentInfo.pSwapchains = m_Swapchains.data();
    presentInfo.pImageIndices = &imageIndex;

    utils::CheckVkResult(m_PresentQueue.presentKHR(presentInfo), "Failed to present image");

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

    eastl::vector<vk::DescriptorSetLayoutBinding> bindings = { vpBinding };

    vk::DescriptorSetLayoutCreateInfo createInfo = {};
    createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    createInfo.pBindings = bindings.data();

    // Create descriptor set layout
    m_DescriptorSetLayout.Create(m_DescriptorSetLayout, createInfo, *this);
}

void Device::CreateUniformBuffers()
{
    vk::DeviceSize vpBufferSize = sizeof(UboViewProjection);
    //vk::DeviceSize modelBufferSize = m_ModelUniformAlignment * MAX_OBJECTS;

    const auto& images = m_Swapchain.GetImages();
    // One uniform buffer for each image
    //m_UniformBufferModel.resize(images.size());
    m_UniformBufferViewProjection.resize(images.size());

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
        //m_UniformBufferModel[i].Create(modelCreateInfo, aCreateInfo, *this, m_Allocator);
        m_UniformBufferViewProjection[i].Create(vpCreateInfo, aCreateInfo, *this, m_Allocator);
    }
}

void Device::CreateDescriptorPool()
{
    // How many descriptors, not descriptor sets
    vk::DescriptorPoolSize vpPoolSize = {};
    vpPoolSize.type = vk::DescriptorType::eUniformBuffer;
    vpPoolSize.descriptorCount = static_cast<uint32_t>(m_UniformBufferViewProjection.size());

    //vk::DescriptorPoolSize modelPoolSize = {};
    //modelPoolSize.type = vk::DescriptorType::eUniformBufferDynamic;
    //modelPoolSize.descriptorCount = static_cast<uint32_t>(m_UniformBufferModel.size());

    eastl::vector<vk::DescriptorPoolSize> poolSizes = { vpPoolSize };

    // Pool creation
    vk::DescriptorPoolCreateInfo poolCreateInfo = {};
    // Maximum number of descriptor sets that can be created from the pool
    poolCreateInfo.maxSets = static_cast<uint32_t>(m_Swapchain.GetImages().size());
    // Number of pool sizes being passed 
    poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolCreateInfo.pPoolSizes = poolSizes.data();

    // Create descriptor pool
    m_DescriptorPool.Create(m_DescriptorPool, poolCreateInfo, *this);
}

void Device::CreateDescriptorSets()
{
    size_t ubSize = m_Swapchain.GetImages().size();
    // One descriptor set for every uniform buffer
    m_DescriptorSets.resize(ubSize);

    // Create one copy of our descriptor set layout per buffer
    eastl::vector<vk::DescriptorSetLayout> setLayouts(ubSize, m_DescriptorSetLayout.Get());

    vk::DescriptorSetAllocateInfo allocInfo = {};
    allocInfo.descriptorPool = m_DescriptorPool;
    // Number of sets to allocate
    allocInfo.descriptorSetCount = static_cast<uint32_t>(ubSize);
    // Layouts to use to allocate sets (1:1 relationship)
    allocInfo.pSetLayouts = setLayouts.data();

    utils::CheckVkResult(allocateDescriptorSets(&allocInfo, m_DescriptorSets.data()), 
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
    //modelBufferInfo.range = m_ModelUniformAlignment;

    //vk::WriteDescriptorSet modelSetWrite = {};
    //modelSetWrite.dstBinding = 1;
    //modelSetWrite.dstArrayElement = 0;
    //modelSetWrite.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
    //modelSetWrite.descriptorCount = 1;

    // Update all descriptor set buffer bindings
    for (size_t i = 0; i < ubSize; ++i)
    {
        // Buffer to get data from
        vpBufferInfo.buffer = m_UniformBufferViewProjection[i];
        //modelBufferInfo.buffer = m_UniformBufferModel[i];

        // Current descriptor set 
        vpSetWrite.dstSet = m_DescriptorSets[i];
        //modelSetWrite.dstSet = m_DescriptorSets[i];

        // Update with new buffer info
        vpSetWrite.pBufferInfo = &vpBufferInfo;
        //modelSetWrite.pBufferInfo = &modelBufferInfo;

        eastl::array<vk::WriteDescriptorSet> setWrites = { vpSetWrite };

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
    //m_ModelTransferSpace = (Model*)_aligned_malloc(
    //    m_ModelUniformAlignment * MAX_OBJECTS, 
    //    m_ModelUniformAlignment
    //);

}

void Device::CreatePushConstantRange()
{
    // NO create needed

    // Where push constant is located
    m_PushRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
    m_PushRange.offset = 0;
    m_PushRange.size = sizeof(Model);
}



void Device::UpdateUniformBuffers(uint32_t imageIndex)
{
    // Copy view & projection data
    auto& vpBuffer = m_UniformBufferViewProjection[imageIndex];
    void* data;
    const auto& vpAllocInfo = vpBuffer.GetAllocationInfo();
    vk::DeviceMemory memory = vpAllocInfo.deviceMemory;
    vk::DeviceSize offset = vpAllocInfo.offset;
    mapMemory(memory, offset, sizeof(UboViewProjection), {}, &data);
    memcpy(data, &m_UboViewProjection, sizeof(UboViewProjection));
    unmapMemory(memory);

    // Copy model data
    //size_t numObjects = objects.size();
    //for (size_t i = 0; i < numObjects; ++i)
    //{
    //    Model* model = (Model*)((uint64_t) m_ModelTransferSpace + (i * m_ModelUniformAlignment));
    //    *model = objects[i].GetModel();
    //}

    //auto& modelBuffer = m_UniformBufferModel[imageIndex];
    //auto& modelAllocInfo = modelBuffer.GetAllocationInfo();
    //memory = modelAllocInfo.deviceMemory;
    //offset = modelAllocInfo.offset;
    //mapMemory(memory, offset, m_ModelUniformAlignment * numObjects, {}, &data);
    //memcpy(data, m_ModelTransferSpace, m_ModelUniformAlignment * numObjects);
    //unmapMemory(memory);
}

void Device::Update(float dt)
{
    static float speed = 90.0f;
    static float speed2 = 230.0f;

    const auto& model = objects[0].GetModel().model;
    objects[0].SetModel(glm::rotate(model, glm::radians(speed * dt), { 0.0f, 0.0f,1.0f }));

    const auto& model2 = objects[1].GetModel().model;
    objects[1].SetModel(glm::rotate(model2, glm::radians(speed2 * dt), { 0.0f, 0.0f,1.0f }));
}
