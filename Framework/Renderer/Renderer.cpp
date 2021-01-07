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
    void InitVulkan(std::weak_ptr<Window> winHandle);

    Instance instance = {};
    PhysicalDevice physicalDevice = {};
    Device device = {};

    uint32_t currentFrame = 0;

    

    std::unique_ptr<Mesh> mesh = {};
};

std::unique_ptr<Renderer_Impl> Renderer::impl = {};
void Renderer::Initialize(std::weak_ptr<Window> window)
{
    impl = std::make_unique<Renderer_Impl>();
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
    return impl->instance;
}

Device& Renderer::GetDevice()
{
    return impl->device;
}

PhysicalDevice& Renderer::GetPhysicalDevice()
{
    return impl->physicalDevice;
}


void Renderer_Impl::InitVulkan(std::weak_ptr<Window> winHandle) {
    instance.Create();
    instance.CreateSurface(winHandle);
    physicalDevice.Create();
    physicalDevice.CreateLogicalDevice(device);
}

Renderer_Impl::~Renderer_Impl()
{
    device.Get().waitIdle();

    device.Destroy();
    instance.Destroy();
}

void Renderer_Impl::DrawFrame()
{
    device.DrawFrame(currentFrame);
    currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
}

void Renderer_Impl::Update(float dt)
{
    device.Update(dt);
}


