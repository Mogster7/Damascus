//------------------------------------------------------------------------------
//
// File Name:	Device.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/18/2020
//
//------------------------------------------------------------------------------
#include "Device.h"

#define VMA_IMPLEMENTATION
#define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#include <vk_mem_alloc.h>

namespace dm
{

void Device::CreateAllocator()
{
	DM_ASSERT_MSG(allocator == nullptr, "Creating an existing member");
	VmaAllocatorCreateInfo info = {};
	info.instance = (VkInstance) OwnerGet<Renderer>().instance.VkType();
	info.physicalDevice = (VkPhysicalDevice) OwnerGet<PhysicalDevice>().VkType();
	info.device = (VkDevice) VkType();

	vmaCreateAllocator(&info, &allocator);
}

void Device::Update(float dt)
{
}

void Device::Create(const vk::DeviceCreateInfo& createInfo, PhysicalDevice* inOwner)
{
    IOwned::CreateOwned(inOwner);
	DM_ASSERT_VK(owner->createDevice(&createInfo, nullptr, &VkType()));
	const QueueFamilyIndices& indices = owner->GetQueueFamilyIndices();
	getQueue(indices.graphics.value(), 0, &graphicsQueue);
	getQueue(indices.present.value(), 0, &presentQueue);
	CreateAllocator();
}

int Device::ImageCount() const
{
    return OwnerGet<Renderer>().ImageCount();
}

Swapchain& Device::Swapchain()
{
    return *OwnerGet<Renderer>().swapchain;
}

Descriptors& Device::Descriptors()
{
    return OwnerGet<Renderer>().descriptors;
}

DescriptorPool& Device::DescriptorPool()
{
    return OwnerGet<Renderer>().descriptorPool;
}

int Device::ImageIndex() const
{
    return OwnerGet<Renderer>().imageIndex;
}

void Device::Destroy()
{
    if (created)
    {
        vmaDestroyAllocator(allocator);
        destroy();
        created = false;
    }
}

Device::~Device() noexcept
{
    Destroy();
}



}