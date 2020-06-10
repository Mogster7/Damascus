#include "RenderingStructures.hpp"
#include "Renderer.h"
#include "Window.h"
#include <glfw3.h>

const std::vector<Vertex> Renderer::meshVerts = {
			{{0.4, -0.4, 0.0}, {1.0f, 0.0f, 0.0f}},
			{{0.4, 0.4, 0.0}, {0.0f, 1.0f, 0.0f}},
			{{-0.4, 0.4, 0.0}, {0.0f, 0.0f, 1.0f}},

			{ { -0.4, 0.4, 0.0 }, {0.0f, 0.0f, 1.0f}},
			{ { -0.4, -0.4, 0.0 }, {1.0f, 1.0f, 0.0f} },
			{ { 0.4, -0.4, 0.0 }, {1.0f, 0.0f, 0.0f} }

    };


VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
class SwapChainSupportDetails {
public:
    SwapChainSupportDetails(const vk::PhysicalDevice& m_Device, const vk::SurfaceKHR& m_Surface) :
        capabilities(m_Device.getSurfaceCapabilitiesKHR(m_Surface)),
        formats(m_Device.getSurfaceFormatsKHR(m_Surface)),
        presentModes(m_Device.getSurfacePresentModesKHR(m_Surface))
    {
    }

    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

class QueueFamilyIndices
{
public:
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    bool isComplete()
    {
        return graphics.has_value() & present.has_value();
    }
};

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

void Renderer::Initialize(std::weak_ptr<Window> window)
{
    Get(window);
}

void Renderer::MakeMesh()
{
    Get().mesh = std::make_unique<Mesh>(Get().m_PhysicalDevice, Get().m_Device, meshVerts);
    Get().CreateCommandBuffers();
}


Renderer::Renderer(std::weak_ptr<Window> winHandle) :
    m_Window(winHandle)
{
    InitVulkan();
}

void Renderer::InitVulkan() {
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();


    CreateSwapChain();
    CreateImageViews();
    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateFramebuffers();
    CreateCommandPool();
    CreateSync();
}

Renderer::~Renderer()
{
    m_Device.waitIdle();

    mesh->DestroyVertexBuffer();
    for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
    {
        m_Device.destroyFence(m_DrawFences[i]);
    }

    for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
    {
        m_Device.destroySemaphore(m_RenderFinished[i]);
        m_Device.destroySemaphore(m_ImageAvailable[i]);
    }

    m_Device.destroyCommandPool(m_GraphicsCmdPool);

    for (auto framebuffer : m_Framebuffers)
    {
        m_Device.destroyFramebuffer(framebuffer);
    }

    m_Device.destroyPipeline(m_GraphicsPipeline);
    m_Device.destroyPipelineLayout(m_PipelineLayout);
    m_Device.destroyRenderPass(m_RenderPass);

    for (auto imageView : m_ImageViews)
        m_Device.destroyImageView(imageView);

    m_Device.destroySwapchainKHR(m_Swapchain);
    m_Device.destroy();

    if (enableValidationLayers)
        m_Instance.destroyDebugUtilsMessengerEXT(m_DebugMessenger, nullptr);

    m_Instance.destroySurfaceKHR(m_Surface);
    m_Instance.destroy();
}

static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << '\n' << std::endl;

    return VK_FALSE;
}


static void PopulateDebugCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = vk::DebugUtilsMessengerCreateInfoEXT(
        {},

        vk::DebugUtilsMessageSeverityFlagsEXT(VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT),

        vk::DebugUtilsMessageTypeFlagsEXT(VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT),

        (PFN_vkDebugUtilsMessengerCallbackEXT)DebugCallback
    );
}





bool checkValidationLayerSupport()
{
    std::vector <vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

std::vector<const char*> GetRequiredExtensions()
{
    unsigned glfwExtensionCount = 0;
    const char** requiredExtensions;
    requiredExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(requiredExtensions, requiredExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}



void Renderer::CreateInstance()
{
    auto vkGetInstanceProcAddr = m_DynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    if (enableValidationLayers && !checkValidationLayerSupport())
        throw std::runtime_error("Validation layers requested, but not available");

    vk::ApplicationInfo appInfo("Hello Triangle", VK_MAKE_VERSION(1, 0, 0), "No Engine", VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_0);
    vk::InstanceCreateInfo createInfo({}, &appInfo);

    auto requiredExtensions = GetRequiredExtensions();

    std::cout << "Required Extensions: \n";
    for (const auto& extension : requiredExtensions) {
        std::cout << '\t' << extension << '\n';
    }
    std::cout << '\n';
    createInfo.enabledExtensionCount = (unsigned)requiredExtensions.size();
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();


    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();


        vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        PopulateDebugCreateInfo(debugCreateInfo);
        createInfo.pNext = (vk::DebugUtilsMessengerCreateInfoEXT*) & debugCreateInfo;
    }
    else {
        createInfo.enabledLayerCount = 0;
    }



    std::vector<vk::ExtensionProperties> extensions = vk::enumerateInstanceExtensionProperties();

    std::cout << "Available Extensions: \n";

    for (const auto& extension : extensions) {
        std::cout << '\t' << extension.extensionName << '\n';
    }


    vk::Result result = vk::createInstance(&createInfo, nullptr, &m_Instance);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create m_Instance");
    }
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Instance);
}

void Renderer::SetupDebugMessenger()
{
    if (!enableValidationLayers) return;

    vk::DebugUtilsMessengerCreateInfoEXT info;
    PopulateDebugCreateInfo(info);

    if (m_Instance.createDebugUtilsMessengerEXT(&info, nullptr, &m_DebugMessenger) != vk::Result::eSuccess)
        throw std::runtime_error("Failed to set up debug messenger");
}


QueueFamilyIndices Renderer::FindQueueFamilies(vk::PhysicalDevice m_Device)
{
    QueueFamilyIndices indices;

    std::vector<vk::QueueFamilyProperties> queueFamilies = m_Device.getQueueFamilyProperties();

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
            indices.graphics = i;

        if (m_Device.getSurfaceSupportKHR(i, m_Surface))
            indices.present = i;

        if (indices.isComplete()) break;

        ++i;
    }

    return indices;
}

bool CheckDeviceExtensionSupport(vk::PhysicalDevice m_Device)
{
    std::vector<vk::ExtensionProperties> available = m_Device.enumerateDeviceExtensionProperties();

    std::set<std::string> required(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : available)
    {
        required.erase(extension.extensionName);
    }

    return required.empty();
}


bool Renderer::IsDeviceSuitable(vk::PhysicalDevice device)
{
    QueueFamilyIndices indices = FindQueueFamilies(device);
    if (!indices.isComplete()) return false;

    bool extensionsSupported = CheckDeviceExtensionSupport(device);
    if (!extensionsSupported) return false;

    SwapChainSupportDetails details(device, m_Surface);
    if (details.formats.empty() || details.presentModes.empty()) return false;

    return true;
}

void Renderer::PickPhysicalDevice()
{

    std::vector<vk::PhysicalDevice> devices = m_Instance.enumeratePhysicalDevices();
    if (devices.empty()) throw std::runtime_error("Failed to find GPU with Vulkan support");

    auto it = std::find_if(devices.begin(), devices.end(), [this](const auto& device) -> bool
        {
            return IsDeviceSuitable(device);
        });

    if (it == devices.end())
        throw std::runtime_error("Failed to find a suitable GPU");

    m_PhysicalDevice = (*it);
}

void Renderer::CreateLogicalDevice()
{
    QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> queueFamilies = { indices.graphics.value(), indices.present.value() };

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

    vk::DeviceCreateInfo createInfo(
        {},
        (uint32_t)queueCreateInfos.size(),
        queueCreateInfos.data(),
        0,
        {},
        (uint32_t)deviceExtensions.size(),
        deviceExtensions.data(),
        &deviceFeatures
    );

    if (m_PhysicalDevice.createDevice(&createInfo, nullptr, &m_Device) != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create logical m_Device");


    m_Device.getQueue(indices.graphics.value(), 0, &m_GraphicsQueue);
    m_Device.getQueue(indices.present.value(), 0, &m_PresentQueue);
}

void Renderer::CreateSurface()
{
    if (glfwCreateWindowSurface((VkInstance)m_Instance, m_Window.lock()->GetHandle(), nullptr, (VkSurfaceKHR*)&m_Surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window m_Surface!");
    }
}

vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    // If only 1 format available and is undefined, then this means ALL availableFormats are available (no restrictions)
    if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined)
    {
        return { vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
    }

    // If restricted, search for optimal format
    for (const auto& format : availableFormats)
    {
        if ((format.format == vk::Format::eR8G8B8A8Unorm || format.format == vk::Format::eB8G8R8A8Unorm)
            && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return format;
        }
    }

    // If can't find optimal format, then just return first format
    return availableFormats[0];
}


vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
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


vk::Extent2D Renderer::ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        glm::uvec2 winDims = m_Window.lock()->GetDimensions();
        vk::Extent2D actualExtent = { winDims.x, winDims.y };

        // Clamp the capable extent to the limits defined by the program
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

void Renderer::CreateSwapChain()
{
    SwapChainSupportDetails scSupport(m_PhysicalDevice, m_Surface);

    // Surface format (color depth)
    vk::SurfaceFormatKHR m_SurfaceFormat = ChooseSwapSurfaceFormat(scSupport.formats);
    // Presentation mode (conditions for swapping images)
    vk::PresentModeKHR presentMode = ChooseSwapPresentMode(scSupport.presentModes);
    // Swap extent (resolution of images in the swap chain)
    vk::Extent2D extent = ChooseSwapExtent(scSupport.capabilities);

    // 1 more than the min to not wait on acquiring
    uint32_t imageCount = scSupport.capabilities.minImageCount + 1;
    // Clamp to max if greater than 0
    if (scSupport.capabilities.maxImageCount > 0 &&
        imageCount > scSupport.capabilities.maxImageCount)
    {
        imageCount = scSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo(
        {},
        m_Surface,
        imageCount,
        m_SurfaceFormat.format,
        m_SurfaceFormat.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment
    );

    QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
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

    createInfo.preTransform = scSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    // Old swap chain is default
    m_Swapchain = m_Device.createSwapchainKHR(createInfo);
    if (bool(m_Swapchain) == false)
    {
        throw std::runtime_error("Failed to create swapchain");
    }

    m_Images = m_Device.getSwapchainImagesKHR(m_Swapchain);
    m_ImageFormat = m_SurfaceFormat.format;
    m_Extent = extent;
}

void Renderer::CreateImageViews()
{
    size_t m_ImagesSize = m_Images.size();
    m_ImageViews.resize(m_ImagesSize);

    for (size_t i = 0; i < m_ImagesSize; ++i)
    {
        vk::ImageViewCreateInfo createInfo(
            {},
            m_Images[i],
            vk::ImageViewType::e2D,
            m_ImageFormat,
            vk::ComponentMapping(),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        );

        m_ImageViews[i] = m_Device.createImageView(createInfo);

        if (bool(m_ImageViews[i]) == false)
            throw std::runtime_error("Failed to create image views");
    }
}


vk::ShaderModule Renderer::CreateShaderModule(const std::vector<char>& code) {
    vk::ShaderModuleCreateInfo createInfo(
        {},
        code.size(),
        reinterpret_cast<const uint32_t*>(code.data())
    );

    vk::ShaderModule shaderModule;
    if (m_Device.createShaderModule(&createInfo, nullptr, &shaderModule) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to create shader module");
    }

    return shaderModule;
}

void Renderer::CreateRenderPass()
{
    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = m_ImageFormat;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;					// Number of samples to write for multisampling
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;				// Describes what to do with attachment before rendering
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;			// Describes what to do with attachment after rendering
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;	// Describes what to do with stencil before rendering
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;	// Describes what to do with stencil after rendering

    // Framebuffer data will be stored as an image, but images can be given different data layouts
    // to give optimal use for certain operations
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;			// Image data layout before render pass starts
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;		// Image data layout after render pass (to change to)

    // Attachment reference uses an attachment index that refers to index in the attachment list passed to m_RenderPassCreateInfo
    vk::AttachmentReference colorAttachmentRef(
        0,
        vk::ImageLayout::eColorAttachmentOptimal
    );

    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    vk::RenderPassCreateInfo createInfo(
        {},
        1,
        &colorAttachment,
        1,
        &subpass
    );

    if (m_Device.createRenderPass(&createInfo, nullptr, &m_RenderPass) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create render pass");
    }

    vk::SubpassDependency dependency({});
    // Indices of the dependency for this subpass and the dependent subpass
    // VK_SUBPASS_EXTERNAL refers to the implicit subpass before/after this pass

    // Happens after
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    // Happens before 
    dependency.dstSubpass = 0;

    // Operations to wait on and what stage they occur
    // Wait on swap chain to finish reading from the image to access it
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = {};

    // Operations that should wait on the above operations to finish are in the color attachment writing stage
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

}

void Renderer::CreateGraphicsPipeline()
{
    auto vertSrc = utils::ReadFile("Shaders/vert.spv");
    auto fragSrc = utils::ReadFile("Shaders/frag.spv");

    vk::ShaderModule vertModule = CreateShaderModule(vertSrc);
    vk::ShaderModule fragModule = CreateShaderModule(fragSrc);

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
    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_Extent.width;
    viewport.height = (float)m_Extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor;

    scissor.extent = m_Extent;
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
    rasterizeState.frontFace = vk::FrontFace::eClockwise;	// Winding to determine which side is front
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

    vk::PipelineLayoutCreateInfo layout;
    layout.setLayoutCount = 0;
    layout.pSetLayouts = nullptr;
    layout.pushConstantRangeCount = 0;
    layout.pPushConstantRanges = nullptr;

    if (m_Device.createPipelineLayout(&layout, nullptr, &m_PipelineLayout) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create pipeline layout");
    }


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
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.layout = m_PipelineLayout;							// Pipeline Layout pipeline should use
    pipelineInfo.renderPass = m_RenderPass;							// Render pass description the pipeline is compatible with
    pipelineInfo.subpass = 0;										// Subpass of render pass to use with pipeline


    m_GraphicsPipeline = m_Device.createGraphicsPipeline(vk::PipelineCache(), pipelineInfo);

    if (bool(m_GraphicsPipeline) != VK_TRUE)
    {
        throw std::runtime_error("Failed to create graphics pipeline");
    }
    // Clean up the programs/modules
    m_Device.destroyShaderModule(vertModule);
    m_Device.destroyShaderModule(fragModule);

}

void Renderer::CreateFramebuffers()
{
    size_t imageViewsSize = m_ImageViews.size();
    m_Framebuffers.resize(imageViewsSize);

    for (size_t i = 0; i < imageViewsSize; ++i)
    {
        vk::ImageView attachments[] = {
            m_ImageViews[i]
        };

        vk::FramebufferCreateInfo createInfo;

        createInfo.renderPass = m_RenderPass;
        createInfo.attachmentCount = 1;
        // List of attachments 1 to 1 with render pass
        createInfo.pAttachments = attachments;
        // FB width/height
        createInfo.width = m_Extent.width;
        createInfo.height = m_Extent.height;
        // FB layers
        createInfo.layers = 1;

        utils::CheckVkResult(m_Device.createFramebuffer(&createInfo, nullptr, &m_Framebuffers[i]), "Failed to create framebuffer");
    }
}

void Renderer::CreateCommandPool()
{
    QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

    vk::CommandPoolCreateInfo poolInfo;
    poolInfo.queueFamilyIndex = indices.graphics.value();

    utils::CheckVkResult(m_Device.createCommandPool(&poolInfo, nullptr, &m_GraphicsCmdPool), "Failed to create command pool");
}

void Renderer::CreateCommandBuffers()
{
    size_t fbSize = m_Framebuffers.size();
    m_CommandBuffers.resize(fbSize);

    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = m_GraphicsCmdPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = static_cast<uint32_t>(fbSize);

    utils::CheckVkResult(m_Device.allocateCommandBuffers(&allocInfo, m_CommandBuffers.data()), "Failed to allocate command buffers");

    vk::ClearValue clearColor(vk::ClearColorValue(std::array<float, 4>{0.6f, 0.65f, 0.4f, 1.0f}));

    vk::RenderPassBeginInfo m_RenderPassInfo;
    m_RenderPassInfo.renderPass = m_RenderPass;
    m_RenderPassInfo.renderArea.extent = m_Extent;
    m_RenderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
    m_RenderPassInfo.clearValueCount = 1;
    m_RenderPassInfo.pClearValues = &clearColor;

    // Record command buffers
    for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
    {
        vk::CommandBufferBeginInfo info(
            {},
            nullptr
        );

        utils::CheckVkResult(m_CommandBuffers[i].begin(&info), "Failed to begin recording command buffer");
        m_RenderPassInfo.framebuffer = m_Framebuffers[i];

        m_CommandBuffers[i].beginRenderPass(&m_RenderPassInfo, vk::SubpassContents::eInline);
        m_CommandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, m_GraphicsPipeline);

        // Buffers to bind
        vk::Buffer vertexBuffers[] = { mesh->GetVertexBuffer() };
        // Offsets
        vk::DeviceSize offsets[] = { 0 };

        m_CommandBuffers[i].bindVertexBuffers(0, 1, vertexBuffers, offsets);

        m_CommandBuffers[i].draw(static_cast<uint32_t>(mesh->GetVertexCount()), 1, 0, 0);
        m_CommandBuffers[i].endRenderPass();
        m_CommandBuffers[i].end();
    }
}


void Renderer::CreateSync()
{
    m_ImageAvailable.resize(MAX_FRAME_DRAWS);
    m_RenderFinished.resize(MAX_FRAME_DRAWS);
    m_DrawFences.resize(MAX_FRAME_DRAWS);

    vk::SemaphoreCreateInfo semInfo{};
    vk::FenceCreateInfo fenceInfo{};
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
    {
        utils::CheckVkResult(m_Device.createSemaphore(&semInfo, nullptr, &m_ImageAvailable[i]), "Failed to create semaphores");
        utils::CheckVkResult(m_Device.createSemaphore(&semInfo, nullptr, &m_RenderFinished[i]), "Failed to create semaphores");
        utils::CheckVkResult(m_Device.createFence(&fenceInfo, nullptr, &m_DrawFences[i]), "Failed to create fences");
    }

}


void Renderer::Impl_DrawFrame()
{
    uint32_t imageIndex;

    // Freeze until previous image is drawn
    m_Device.waitForFences(1, &m_DrawFences[m_CurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>().max());
    // Close the fence for this current frame again
    m_Device.resetFences(1, &m_DrawFences[m_CurrentFrame]);

    m_Device.acquireNextImageKHR(m_Swapchain, std::numeric_limits<uint64_t>().max(), m_ImageAvailable[m_CurrentFrame], nullptr, &imageIndex);

    std::array<vk::PipelineStageFlags, 1> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

    vk::SubmitInfo submitInfo;

    submitInfo.waitSemaphoreCount = 1;
    // Semaphores to wait on 
    submitInfo.pWaitSemaphores = &m_ImageAvailable[m_CurrentFrame];
    // Stages to check semaphores at
    submitInfo.pWaitDstStageMask = waitStages.data();

    // Number of command buffers to submit
    submitInfo.commandBufferCount = 1;
    // Command buffers for submission
    submitInfo.pCommandBuffers = &m_CommandBuffers[imageIndex];

    submitInfo.signalSemaphoreCount = 1;
    // Signals command buffer is finished
    submitInfo.pSignalSemaphores = &m_RenderFinished[m_CurrentFrame];


    utils::CheckVkResult(m_GraphicsQueue.submit(1, &submitInfo, m_DrawFences[m_CurrentFrame]), "Failed to submit draw command buffer");

    vk::PresentInfoKHR presentInfo{};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_RenderFinished[m_CurrentFrame];

    std::array<vk::SwapchainKHR, 1> swapchains = { m_Swapchain };
    presentInfo.swapchainCount = (uint32_t)swapchains.size();
    presentInfo.pSwapchains = swapchains.data();
    presentInfo.pImageIndices = &imageIndex;

    utils::CheckVkResult(m_PresentQueue.presentKHR(presentInfo), "Failed to present image");
    //m_PresentQueue.waitIdle();
    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAME_DRAWS;

}



