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
    CreateAllocator();
    CreateSwapchain();
    CreateCommandPool();
    CreateDepthBuffer();
    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateFramebuffers();
    CreateCommandBuffers();
    CreateSync();
}

void Device::CreateAllocator()
{
    ASSERT(allocator == nullptr, "Creating an existing member");
    VmaAllocatorCreateInfo info = {};
    info.instance = GetInstance();
    info.physicalDevice = *m_owner;
    info.device = *this;

    vmaCreateAllocator(&info, &allocator);
}

void Device::Destroy()
{
    waitIdle();

    for(auto& fence : drawFences)
        fence.Destroy();

    for (size_t i = 0; i < renderFinished.size(); ++i)
    {
        renderFinished[i].Destroy();
        imageAvailable[i].Destroy();
    }

	freeCommandBuffers(commandPool.Get(),
					   drawBuffers.size(),
					   drawBuffers.data());
    commandPool.Destroy();
    for (auto& fb : frameBuffers)
        fb.Destroy();

    pipeline.Destroy();
    pipelineLayout.Destroy();

    renderPass.Destroy();
    depth.Destroy();
    swapchain.Destroy();
    vmaDestroyAllocator(allocator);

    destroy();
}

void Device::CreateSwapchain(bool recreate)
{    
	if (recreate) swapchain.Destroy();
	
    vk::PhysicalDevice physDevice = *m_owner;
    vk::SurfaceKHR surface = GetInstance().GetSurface();

    vk::SurfaceCapabilitiesKHR capabilities = physDevice.getSurfaceCapabilitiesKHR(surface);
    std::vector<vk::SurfaceFormatKHR> formats = physDevice.getSurfaceFormatsKHR(surface);
    std::vector<vk::PresentModeKHR> presentModes = physDevice.getSurfacePresentModesKHR(surface);

    // Surface format (color depth)
    vk::SurfaceFormatKHR format = Swapchain::ChooseSurfaceFormat(formats);
    // Presentation mode (conditions for swapping images)
    vk::PresentModeKHR presentMode = Swapchain::ChoosePresentMode(presentModes);
    // Swap extent (resolution of images in the swap chain)
    vk::Extent2D extent = Swapchain::ChooseExtent(GetInstance().GetWindow().lock()->GetDimensions(),
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
        GetInstance().GetSurface(),
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
    colorAttachment.loadOp = vk::AttachmentLoadOp::eLoad;				// Describes what to do with attachment before rendering
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;			// Describes what to do with attachment after rendering
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;	// Describes what to do with stencil before rendering
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;	// Describes what to do with stencil after rendering

    // Framebuffer data will be stored as an image, but images can be given different data layouts
    // to give optimal use for certain operations
    colorAttachment.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;			// Image data layout before render pass starts
    colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;		// Image data layout after render pass (to change to)

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
    clearValues[0].color = vk::ClearColorValue(std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f});
	clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f);

    renderPass.Create(createInfo, *this, swapchain.extent, clearValues);
}

void Device::CreateGraphicsPipeline()
{
   }

void Device::CreateDepthBuffer(bool recreate)
{
	if (recreate) depth.Destroy();

    auto& extent = swapchain.extent;
	depth.Create(FrameBufferAttachment::GetDepthFormat(), { extent.width, extent.height },
						 vk::ImageUsageFlagBits::eDepthStencilAttachment |
						 vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc,
						 vk::ImageAspectFlagBits::eDepth,
						 //vk::ImageLayout::eTransferSrcOptimal,
						 vk::ImageLayout::eDepthStencilAttachmentOptimal,

						 // No sampler
						 *this);
}

void Device::CreateFramebuffers(bool recreate)
{
    if (recreate)
    {
		for (auto& fb : frameBuffers)
			fb.Destroy();
    }

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
    createInfo.renderPass = renderPass.Get();
    createInfo.attachmentCount = 2;


    vk::ImageView depthView = depth.imageView;
    for (size_t i = 0; i < imageViewsSize; ++i)
    {
        std::array<vk::ImageView, 2> attachments = {
            imageViews[i].Get(),
            depthView
        };

        // List of attachments 1 to 1 with render pass
        createInfo.pAttachments = attachments.data();

        frameBuffers[i].Create(&frameBuffers[i], createInfo, *this);
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
						   drawBuffers.size(),
						   drawBuffers.data());
    }
	
    size_t fbSize = frameBuffers.size();
    drawBuffers.resize(fbSize);

    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = commandPool.Get();
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = static_cast<uint32_t>(fbSize);

    utils::CheckVkResult(allocateCommandBuffers(&allocInfo, drawBuffers.data()), "Failed to allocate command buffers");
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

void Device::RecreateSurface()
{
    waitIdle();

    CreateSwapchain(true);
    CreateDepthBuffer(true);
    CreateCommandBuffers(true);

    waitIdle();
}

void Device::Update(float dt)
{
}

bool Device::PrepareFrame(const uint32_t frameIndex)
 {
    auto result = acquireNextImageKHR(swapchain.Get(), 
                                      std::numeric_limits<uint64_t>::max(), 
                                      imageAvailable[frameIndex].Get(),
                                      nullptr, &imageIndex);
    // Freeze until previous image is drawn
    waitForFences(1, &drawFences[frameIndex], VK_TRUE, std::numeric_limits<uint64_t>::max());
    // Close the fence for this current frame again
    resetFences(1, &drawFences[frameIndex]);
    // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
    if ((result == vk::Result::eErrorOutOfDateKHR) || (result == vk::Result::eSuboptimalKHR)) {

		// Return if surface is out of date
        return true;
    }
    else {
        utils::CheckVkResult(result, "Failed to prepare render frame");
        return false;
    }
}


// Return if surface is out of date
bool Device::SubmitFrame(const uint32_t frameIndex, vk::Semaphore waitSemaphore)
{
    vk::PresentInfoKHR presentInfo{};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pWaitSemaphores = &waitSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    vk::Result result = graphicsQueue.presentKHR(presentInfo);

    if (!((result == vk::Result::eSuccess) || (result == vk::Result::eSuboptimalKHR))) {
        if (result == vk::Result::eErrorOutOfDateKHR) {
			// Return if surface is out of date
            // Swap chain is no longer compatible with the surface and needs to be recreated
            return true;
        }
        else {
            utils::CheckVkResult(result, "Failed to submit frame");
        }
    }
	return false;
}
