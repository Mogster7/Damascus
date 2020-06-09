//------------------------------------------------------------------------------
//
// File Name:	Renderer.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/5/2020 
//
//------------------------------------------------------------------------------
#pragma once


class Window;
class QueueFamilyIndices;
class Vertex;
class Mesh;

class Renderer
{

public:
    static void Initialize(std::weak_ptr<Window> window);
    static void DrawFrame() { Get().Impl_DrawFrame(); }
    

private:
    Renderer(std::weak_ptr<Window> winHandle);
    ~Renderer();
    static Renderer& Get(std::weak_ptr<Window> winHandle = {})
    {
        static Renderer renderer(winHandle);
        return renderer;
    }

    // Utils
    QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice device);
    bool IsDeviceSuitable(vk::PhysicalDevice device);
    vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    vk::ShaderModule CreateShaderModule(const std::vector<char>& code);
    

    // Init functions
    void InitVulkan();
    void CreateInstance();
    void SetupDebugMessenger();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapChain();
    void CreateImageViews();
    void CreateRenderPass();
    void CreateGraphicsPipeline();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSync();

    // Draw
    void Impl_DrawFrame();


    // Constants
    const uint32_t MAX_FRAME_DRAWS = 2;

    std::weak_ptr<Window> m_Window;

    vk::SurfaceKHR m_Surface;
    vk::Instance m_Instance = nullptr;
    vk::DynamicLoader m_DynamicLoader = {};

    uint32_t m_CurrentFrame = 0;

    vk::Device m_Device = nullptr;
    vk::PhysicalDevice m_PhysicalDevice = nullptr;

    vk::Queue m_GraphicsQueue;
    vk::Queue m_PresentQueue;

    vk::SwapchainKHR m_Swapchain;
    // Uses same index as framebuffer & command buffer
    std::vector<vk::Image> m_Images;
    vk::Format m_ImageFormat;
    vk::Extent2D m_Extent;

    // Uses same index as swap chain image & command buffer
    std::vector<vk::Framebuffer> m_Framebuffers;

    std::vector<vk::ImageView> m_ImageViews;

    vk::RenderPass m_RenderPass;
    vk::PipelineLayout m_PipelineLayout;
    vk::Pipeline m_GraphicsPipeline;

    vk::CommandPool m_GraphicsCmdPool;
    // Uses same index as swap chain framebuffer & image
    std::vector <vk::CommandBuffer> m_CommandBuffers;

    std::vector<vk::Semaphore> m_ImageAvailable;
    std::vector<vk::Semaphore> m_RenderFinished;
    std::vector<vk::Fence> m_DrawFences;

    vk::DebugUtilsMessengerEXT m_DebugMessenger = nullptr;
    
    static const std::vector<Vertex> meshVerts;

    std::unique_ptr<Mesh> mesh = {};


    friend class Window;
};





