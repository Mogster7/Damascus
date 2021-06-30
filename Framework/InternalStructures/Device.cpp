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
}

void Device::CreateAllocator()
{
    ASSERT(allocator == nullptr, "Creating an existing member");
    VmaAllocatorCreateInfo info = {};
    info.instance = (VkInstance) GetInstance().Get();
    info.physicalDevice = (VkPhysicalDevice) m_owner->Get();
    info.device = (VkDevice) Get();

    vmaCreateAllocator(&info, &allocator);
}

void Device::Destroy()
{
    vmaDestroyAllocator(allocator);
    destroy();
}

void Device::RecreateSurface()
{
}

void Device::Update(float dt)
{
}



