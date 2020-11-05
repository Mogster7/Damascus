#include "Renderer.h"
#include "Window.h"
#include <glfw3.h>

#include <vk_mem_alloc.h>



VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

struct Renderer_Impl
{    
    Renderer_Impl() = default;
    ~Renderer_Impl();
    
    // Draw
    void DrawFrame();

     // Update
    void Update(float dt);

    
    // Init functions
    void InitVulkan(eastl::weak_ptr<Window> winHandle);

    Instance m_Instance = {};
    PhysicalDevice m_PhysicalDevice = {};
    Device m_Device = {};

    uint32_t m_CurrentFrame = 0;

    

    eastl::unique_ptr<Mesh> mesh = {};
};

eastl::unique_ptr<Renderer_Impl> Renderer::impl = {};
void Renderer::Initialize(eastl::weak_ptr<Window> window)
{
    impl = eastl::make_unique<Renderer_Impl>();
    impl->InitVulkan(window);
}

void Renderer::Destroy()
{
    impl.reset();
}

void Renderer::Update()
{
    float time = glfwGetTime();
    Renderer::dt = time - Renderer::time;
    Renderer::time = time;

    impl->Update(dt);
}

void Renderer::DrawFrame()
{
    impl->DrawFrame();
}

Instance& Renderer::GetInstance()
{   
    return impl->m_Instance;
}

Device& Renderer::GetDevice()
{
    return impl->m_Device;
}

PhysicalDevice& Renderer::GetPhysicalDevice()
{
    return impl->m_PhysicalDevice;
}


void Renderer_Impl::InitVulkan(eastl::weak_ptr<Window> winHandle) {
    m_Instance.Create();
    m_Instance.CreateSurface(winHandle);
    m_PhysicalDevice.Create();
    m_PhysicalDevice.CreateLogicalDevice(m_Device);
    //m_Device.CreateSwapchain();
    //m_Device.GetSwapchain().CreateImageViews();
    //m_Device.CreateRenderPass();
    //m_Device.CreateDescriptorSetLayout();
    //m_Device.CreateGraphicsPipeline();
    //m_Device.CreateFramebuffers();
    //m_Device.CreateCommandPool();

    //m_Device.CreateCommandBuffers();
    //m_Device.CreateUniformBuffers();
    //m_Device.RecordCommandBuffers();

    //m_Device.CreateSync();
}

Renderer_Impl::~Renderer_Impl()
{
    m_Device.Get().waitIdle();

    m_Device.Destroy();
    m_Instance.Destroy();
}

void Renderer_Impl::DrawFrame()
{
    m_Device.DrawFrame(m_CurrentFrame);
    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAME_DRAWS;
}

void Renderer_Impl::Update(float dt)
{
    m_Device.Update(dt);
}


