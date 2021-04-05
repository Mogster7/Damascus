#include "RenderingContext.h"
#include "RenderingContext_Impl.h"
#include "Window.h"
#include <glfw3.h>
#include <vk_mem_alloc.h>


VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

void RenderingContext::Update()
{
	static float prevTime = glfwGetTime();
	float time = glfwGetTime();
	dt = time - prevTime;

	device.Update(dt);
	prevTime = time;
}

	
void RenderingContext::Initialize(std::weak_ptr<Window> winHandle, bool enableOverlay) 
{
    this->window = winHandle;
    instance.Create();
    instance.CreateSurface(winHandle);
    physicalDevice.Create(instance);
    physicalDevice.CreateLogicalDevice(device);
	enabledOverlay = enableOverlay;
	InitializeMeshStatics(device);
	if (enabledOverlay)
	{
		overlay.Create(window, instance, physicalDevice, device);
	}
}

void RenderingContext::Destroy()
{
	if (enabledOverlay)
		overlay.Destroy();
	device.Destroy();
	instance.Destroy();
}


