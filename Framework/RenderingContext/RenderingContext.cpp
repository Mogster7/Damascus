#include "RenderingContext.h"
#include "RenderingContext_Impl.h"
#include "Window.h"
#include <glfw3.h>
#include <vk_mem_alloc.h>




VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

RenderingContext_Impl* RenderingContext::impl = {};
void RenderingContext::Initialize(std::weak_ptr<Window> window)
{
    impl = new RenderingContext_Impl();
    impl->InitVulkan(window);
}

void RenderingContext::Destroy()
{
    delete impl;
}

void RenderingContext::Update()
{
    float time = glfwGetTime();
    RenderingContext::dt = time - RenderingContext::time;
    RenderingContext::time = time;

    impl->Update(dt);
}

void RenderingContext::DrawFrame()
{
    impl->DrawFrame();
}

Instance& RenderingContext::GetInstance()
{   
    return impl->instance;
}

Device& RenderingContext::GetDevice()
{
    return impl->device;
}

PhysicalDevice& RenderingContext::GetPhysicalDevice()
{
    return impl->physicalDevice;
}



