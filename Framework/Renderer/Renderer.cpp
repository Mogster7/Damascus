#include <SDL.h>
#include <utility>
#include <vk_mem_alloc.h>
#include "Renderer.h"


VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
namespace dm
{

IRenderingContext::IRenderingContext(Renderer& inRenderer)
    : renderer(inRenderer)
{
    vk::SemaphoreCreateInfo ci = {};
    for(int i = 0 ; i < MAX_FRAME_DRAWS; ++i)
    {
        ready[i].Create(ci, &renderer.device);
        finished[i].Create(ci, &renderer.device);
    }
}

void Renderer::Create(std::weak_ptr<Window> inWindow)
{
    instance.Create(std::move(inWindow));
    physicalDevice.Create(this);
    CreateDevice();
    descriptors.Create(&device);
    CreateSwapchain();
 //   device.setsToFree.resize(ImageCount());
    CreateSync();
    CreateCommandPool();
    CreateCommandBuffers();
    CreateDescriptorPool();
    InitializeMeshStatics(&device);
    created = true;
}

void Renderer::Update(float dt)
{
    device.Update(dt);

    for (auto& [id, context] : renderingContexts)
    {
        context->Update(dt);
    }
}

void Renderer::Render(){

    // - ACQUIRE AN IMAGE FROM THE SWAP CHAIN
    if (PrepareFrame())
    {
        RecreateSwapchain();
        return;
    }

    descriptors.globalSet->WriteSet();
    for(auto& [id, context] : renderingContexts)
        context->AssignGlobalUniform(*descriptors.globalSet);

    // - SUBMIT COMMAND BUFFERS FOR EXECUTION
    vk::PipelineStageFlags stageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    vk::SubmitInfo beginSubmit = {};
    beginSubmit.pWaitDstStageMask = &stageFlags;
    beginSubmit.waitSemaphoreCount = 1;
    beginSubmit.signalSemaphoreCount = 1;
    beginSubmit.pWaitSemaphores = &imageAvailable[frameIndex];

    // Update Uniforms
    vk::CommandBufferBeginInfo beginInfo = {};
    DM_ASSERT_VK(commandBuffers[frameIndex].begin(&beginInfo));
    descriptors.globalSetData.UpdateBindings(commandBuffers[frameIndex], imageIndex);
    commandBuffers[frameIndex].end();
    beginSubmit.pCommandBuffers = &commandBuffers[frameIndex];
    beginSubmit.commandBufferCount = 1;

    std::vector<vk::SubmitInfo> submitInfo;

    int i = 0, submitIndex = 0;
    for (; i < renderingContexts.size() + 1; ++i)
    {
        IRenderingContext* context = nullptr;
        bool last = i == renderingContexts.size();
        bool first = i == 0;

        auto& submit = submitInfo.emplace_back();
        submit = beginSubmit;

        // Connecting two contexts, relay finish of previous context to ready of new context
        if (!first)
        {
            auto& [prevId, prevContext] = renderingContexts[i-1];

            submitInfo[submitIndex].waitSemaphoreCount = 1;
            submitInfo[submitIndex].commandBufferCount = 0;
            submitInfo[submitIndex].pWaitSemaphores = &prevContext->finished[frameIndex].VkType();
        }

        if (last)
        {
            submitInfo[submitIndex].pSignalSemaphores = &renderFinished[frameIndex].VkType();
        }
        else
        {
            context = renderingContexts[i].second;
            submitInfo[submitIndex].pSignalSemaphores = &context->ready[frameIndex].VkType();

            std::vector<vk::SubmitInfo> contextSubmissions = context->Record();
            submitInfo.insert(submitInfo.end(), contextSubmissions.begin(), contextSubmissions.end());
            submitIndex += (int)contextSubmissions.size();
        }

        ++submitIndex;
    }


    SubmitFrame(submitInfo.size(), submitInfo.data(), frameResourcesInUse[frameIndex].VkType());
    device.waitIdle();

    // - RETURN THE IMAGE TO THE SWAP CHAIN FOR PRESENTATION

    if (PresentFrame())
    {
        RecreateSwapchain();
        return;
    }

    // Advance to next frame
    frameIndex = (frameIndex + 1) % dm::MAX_FRAME_DRAWS;
}


void Renderer::CreateDevice()
{
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> queueFamilies = { physicalDevice.queueFamilyIndices.graphics.value(), physicalDevice.queueFamilyIndices.present.value() };

    float queuePriority = 1.0f;
    queueCreateInfos.reserve(queueFamilies.size());

    for (uint32_t queueFamily : queueFamilies)
    {
        queueCreateInfos.emplace_back(
            vk::DeviceQueueCreateFlags(),
            queueFamily,
            1,
            &queuePriority);
    }

    vk::PhysicalDeviceFeatures deviceFeatures{};
    //deviceFeatures.wideLines = VK_TRUE;
    //deviceFeatures.fillModeNonSolid = VK_TRUE;

    vk::DeviceCreateInfo createInfo(
        {},
        (uint32_t) queueCreateInfos.size(),
        queueCreateInfos.data(),
        0,
        {},
        (uint32_t) deviceExtensions.size(),
        deviceExtensions.data(),
        &deviceFeatures);

    device.Create(createInfo, &physicalDevice);
}

void Renderer::CreateSwapchain()
{
    vk::SurfaceKHR surface = instance.surface;

    vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    std::vector<vk::SurfaceFormatKHR> formats = physicalDevice.getSurfaceFormatsKHR(surface);
    std::vector<vk::PresentModeKHR> presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

    // Surface format (color depth)
    vk::SurfaceFormatKHR format = Swapchain::ChooseSurfaceFormat(formats);
    // Presentation mode (conditions for swapping images)
    vk::PresentModeKHR presentMode = Swapchain::ChoosePresentMode(presentModes);
    // Swap extent (resolution of images in the swap chain)
    vk::Extent2D extent = Swapchain::ChooseExtent(instance.window.lock()->GetDimensions(), capabilities);

    // 1 more than the min to not wait on acquiring
    uint32_t imageCount;
    imageCount = capabilities.minImageCount + 1;

    // Clamp to max if greater than 0
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
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
        vk::ImageUsageFlagBits::eColorAttachment);


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
        createInfo.queueFamilyIndexCount = 0;    // Optional
        createInfo.pQueueFamilyIndices = nullptr;// Optional
    }

    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = (!oldSwapchain) ? nullptr : oldSwapchain->VkType();

    // Store retired swap-chain, so it's not deleted before we create the new swap chain.
    oldSwapchain.swap(swapchain);

    // Create new swapchain.
    swapchain = std::make_shared<Swapchain>();
    swapchain->Create(createInfo, format.format, extent, &device);

    // Set old swapchain for next iteration when recreating the pipeline.
    oldSwapchain = swapchain;
}

void Renderer::RecreateSwapchain()
{
    // Pause window until a new event is registered.
    while ((SDL_GetWindowFlags(instance.window.lock()->GetHandle()) & SDL_WINDOW_MINIMIZED))
    {
        SDL_Event event;
        SDL_WaitEvent(&event);
    }

    // Wait for resources in use
    device.waitIdle();

    // Create swap-chain based on old swap-chain and clean pu old swap chain dependent resources.
    CreateSwapchain();

    CreateCommandBuffers();

    // Clean up swap-chain dependent resources and recreate them referencing new swapchain.
    for(auto& [id, context] : renderingContexts)
        context->OnRecreateSwapchain();

    // Have enough amount of fences handles for swap-chain images in use synchronization.
    imagesInUse.resize(ImageCount());
}
void Renderer::CreateSync()
{
    vk::SemaphoreCreateInfo semInfo{};
    vk::FenceCreateInfo fenceInfo{};
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    frameResourcesInUse = std::vector<Fence>(MAX_FRAME_DRAWS);
    imagesInUse = std::vector<Fence>(ImageCount());
    imageAvailable = std::vector<Semaphore>(MAX_FRAME_DRAWS);
    renderFinished = std::vector<Semaphore>(MAX_FRAME_DRAWS);

    for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
    {
        imageAvailable[i].Create(semInfo, &device);
        renderFinished[i].Create(semInfo, &device);
        frameResourcesInUse[i].Create(fenceInfo, &device);
    }
}


void Renderer::CreateCommandPool()
{
    QueueFamilyIndices indices = physicalDevice.GetQueueFamilyIndices();

    vk::CommandPoolCreateInfo poolInfo;
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolInfo.queueFamilyIndex = indices.graphics.value();

    commandPool.Create(poolInfo, &device);
}

bool Renderer::PrepareFrame()
{
    // Wait for previous frame to be finished (And not-in-use)
    DM_ASSERT_VK(device.waitForFences(1, frameResourcesInUse[frameIndex].VkTypePtr(), VK_TRUE, std::numeric_limits<uint64_t>::max())); // Wait for all fences and no timeout

    // Acquire next image index, signal imageAvailable when it is done
    vk::Result result;
    try
    {
         result = device.acquireNextImageKHR(
            swapchain->VkType(),
            std::numeric_limits<uint64_t>::max(),
            imageAvailable[frameIndex].VkType(),
            nullptr,
            (uint32_t*) &imageIndex);
    }
    catch (...){}

    // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
    if ((result == vk::Result::eErrorOutOfDateKHR) || (result == vk::Result::eSuboptimalKHR))
    {
        // Return if surface is out of date
        return true;
    }
    else
    {
        DM_ASSERT_VK(result);
    }

    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (imagesInUse[imageIndex].created)
    {
        DM_ASSERT_VK(device.waitForFences(1, imagesInUse[imageIndex].VkTypePtr(), VK_TRUE, UINT64_MAX));
    }

    // Mark the image as now being in use by given frame in use
    imagesInUse[imageIndex].VkType() = frameResourcesInUse[frameIndex].VkType();
    imagesInUse[imageIndex].created = true;

    // Kept freeing behavior for descriptor sets, if we need it in the future
//    if (!device.setsToFree[imageIndex].empty())
//    {
//        for(auto& sets : device.setsToFree[imageIndex])
//        {
//            device.freeDescriptorSets(descriptorPool.VkType(),
//                                      sets.size(),
//                                      sets.data());
//        }
//        device.setsToFree[imageIndex].clear();
//    }

    //    DM_ASSERT_VK(device.resetFences(1, &frameResourcesInUse[frameIndex]));
    return false;
}

void Renderer::SubmitFrame(unsigned submitInfoCount, vk::SubmitInfo* submitInfo, vk::Fence fence) const
{
    // Reset frame fences since we are about to submit the given frame
    DM_ASSERT_VK(device.resetFences(1, &fence));

    // Submit graphic command buffer queue for given frame
    DM_ASSERT_VK(device.graphicsQueue.submit(submitInfoCount, submitInfo, fence));
}

bool Renderer::PresentFrame()
{
    vk::PresentInfoKHR presentInfo{};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchain.get();
    presentInfo.pImageIndices = (const unsigned*) &imageIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinished[frameIndex];

    vk::Result result {};

    try
    {
        // Submit the request to present an image to the swap chain
        result = device.graphicsQueue.presentKHR(presentInfo);
    }
    catch (...)
    {
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
                DM_ASSERT_VK(result);
            }
        }
    }

    // Recreate swap chain in window was resized.
    if (framebufferResize)
    {
        framebufferResize = false;
        return true;
    }

    // Continue as expected.
    return false;
}

int Renderer::ImageCount() const
{
    return swapchain->ImageCount();
}

Renderer::~Renderer()
{
    if (created)
    {
        device.waitIdle();
        DestroyMeshStatics();

        for(auto& image : imagesInUse)
            image.created = false;

//        for (auto& fences : frameResourcesInUse)
//            fences.created = false;

        for (auto& [id, context] : renderingContexts)
        {
            delete context;
        }
        renderingContexts.clear();
        created = false;
    }
}
void Renderer::CreateDescriptorPool()
{
    vk::DescriptorPoolCreateInfo poolInfo = {};
    poolInfo.maxSets = 100000 * device.ImageCount();
    const unsigned itemSize = 4096 * 16 * device.ImageCount();
    std::vector<vk::DescriptorPoolSize> poolSizes = {
        { vk::DescriptorType::eSampler, itemSize },
        { vk::DescriptorType::eCombinedImageSampler, itemSize },
        { vk::DescriptorType::eSampledImage, itemSize },
        { vk::DescriptorType::eStorageImage, itemSize },
        { vk::DescriptorType::eUniformTexelBuffer, itemSize },
        { vk::DescriptorType::eStorageTexelBuffer, itemSize },
        { vk::DescriptorType::eUniformBuffer, itemSize },
        { vk::DescriptorType::eStorageBuffer, itemSize },
        { vk::DescriptorType::eUniformBufferDynamic, itemSize },
        { vk::DescriptorType::eStorageBufferDynamic, itemSize },
        { vk::DescriptorType::eInputAttachment, itemSize }
    };
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    //poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    descriptorPool.Create(poolInfo, &device);
}

void Renderer::CreateCommandBuffers()
{
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.commandPool = commandPool.VkType();
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = ImageCount();

    commandBuffers.Create(allocInfo, &commandPool);
}


}// namespace dm