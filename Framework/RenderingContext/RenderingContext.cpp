#include "RenderingContext.h"
#include "Window.h"

#include <SDL.h>
#include <utility>
#include <vk_mem_alloc.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
namespace dm
{

void Renderer::Create(std::weak_ptr<Window> inWindow)
{
    instance.Create(std::move(inWindow));
    physicalDevice.Create(this);
    CreateDevice();
    CreateSwapchain();
    CreateSync();
    CreateCommandPool();
    InitializeMeshStatics(&device);
    created = true;
}

void Renderer::Update(float dt)
{
    for (RenderingContext* context : renderingContexts)
    {
        context->Update(dt);
    }
}

void Renderer::Render()
{
    vk::SubmitInfo submitInfo;
    PrepareFrame();

    std::vector<vk::CommandBuffer> commandBuffers;
    vk::PipelineStageFlags stageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    submitInfo.pWaitDstStageMask = &stageFlags;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailable[frameIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinished[frameIndex];

    for (RenderingContext* context : renderingContexts)
    {
        commandBuffers.emplace_back(context->Record());
    }

    submitInfo.commandBufferCount = commandBuffers.size();
    submitInfo.pCommandBuffers = commandBuffers.data();

    SubmitFrame(1, &submitInfo, drawFences[frameIndex]);
    PresentFrame();

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
        &deviceFeatures);

    device.Create(createInfo, &physicalDevice);
}

void Renderer::CreateSwapchain(bool recreate)
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
    vk::Extent2D extent = Swapchain::ChooseExtent(instance.window.lock()->GetDimensions(), capabilities);

    // 1 more than the min to not wait on acquiring
    uint32_t imageCount = capabilities.minImageCount + 1;
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
    // Old swap chain is default
    swapchain.Create(createInfo, format.format, extent, &device);
}

void Renderer::CreateSync()
{
    vk::SemaphoreCreateInfo semInfo{};
    vk::FenceCreateInfo fenceInfo{};
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    drawFences = std::vector<Fence>(MAX_FRAME_DRAWS);
    imageAvailable = std::vector<Semaphore>(MAX_FRAME_DRAWS);
    renderFinished = std::vector<Semaphore>(MAX_FRAME_DRAWS);

    for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
    {
        imageAvailable[i].Create(semInfo, &device);
        renderFinished[i].Create(semInfo, &device);
        drawFences[i].Create(fenceInfo, &device);
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
    // Acquire next image index, signal imageAvailable when it is done
    auto result = device.acquireNextImageKHR(
        swapchain.VkType(),
        std::numeric_limits<uint64_t>::max(),
        imageAvailable[frameIndex].VkType(),
        nullptr,
        (uint32_t*) &imageIndex);

    // Freeze until previous frame for this index is drawn
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

void Renderer::SubmitFrame(unsigned submitInfoCount, vk::SubmitInfo* submitInfo, vk::Fence fence) const
{
    DM_ASSERT_VK(device.graphicsQueue.submit(submitInfoCount, submitInfo, fence));
}

bool Renderer::PresentFrame()
{
    vk::PresentInfoKHR presentInfo{};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = (const unsigned*) &imageIndex;
    presentInfo.pWaitSemaphores = &renderFinished[frameIndex];
    presentInfo.waitSemaphoreCount = 1;
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

Renderer::~Renderer()
{
    if (created)
    {
        device.waitIdle();
        DestroyMeshStatics();

        for (auto& context : renderingContexts)
        {
            delete context;
        }
    }
}


}// namespace dm