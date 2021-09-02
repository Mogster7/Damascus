//------------------------------------------------------------------------------
//
// File Name:	PhysicalDevice.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/18/2020
//
//------------------------------------------------------------------------------
#include "PhysicalDevice.h"
#include "RenderingContext.h"

namespace dm {


bool PhysicalDevice::CheckDeviceExtensionSupport(vk::PhysicalDevice device) const
{
	std::vector<vk::ExtensionProperties> available = device.enumerateDeviceExtensionProperties();

	std::set<std::string> required(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : available)
	{
		required.erase(extension.extensionName);
	}

	return required.empty();
}


bool PhysicalDevice::IsDeviceSuitable(vk::PhysicalDevice device) const
{
	auto indices = FindQueueFamilies(&OwnerGet<RenderingContext>(), device);
	if (!indices.isComplete())
		return false;

	bool extensionsSupported = CheckDeviceExtensionSupport(device);
	if (!extensionsSupported)
		return false;

	vk::SurfaceKHR surface = OwnerGet<RenderingContext>().instance.surface;
	if (device.getSurfaceFormatsKHR(surface).empty() ||
		device.getSurfacePresentModesKHR(surface).empty())
		return false;

    vk::PhysicalDeviceProperties physicalDeviceProperties;
    device.getProperties(&physicalDeviceProperties);
    if (physicalDeviceProperties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu)
    {
        return false;
    }

	return true;
}


void PhysicalDevice::Create(RenderingContext* owner)
{
	this->owner = owner;
	std::vector<vk::PhysicalDevice> devices = OwnerGet<RenderingContext>().instance.enumeratePhysicalDevices();
	assert(!devices.empty());

	auto it = std::find_if(devices.begin(), devices.end(), [this](const auto& device) -> bool
	{
		return IsDeviceSuitable(device);
	});

	assert(it != devices.end());

	VkType() = *it;
	queueFamilyIndices = FindQueueFamilies(&OwnerGet<RenderingContext>(), VkType());
	getProperties(&properties);
}



const QueueFamilyIndices& PhysicalDevice::GetQueueFamilyIndices() const
{
	return queueFamilyIndices;
}

vk::DeviceSize PhysicalDevice::GetMinimumUniformBufferOffset() const
{
	return properties.limits.minUniformBufferOffsetAlignment;
}

QueueFamilyIndices PhysicalDevice::FindQueueFamilies(RenderingContext* context, vk::PhysicalDevice pd)
{
	QueueFamilyIndices indices;

	std::vector<vk::QueueFamilyProperties> queueFamilies = pd.getQueueFamilyProperties();

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
			indices.graphics = i;

		if (pd.getSurfaceSupportKHR(i, context->instance.surface))
			indices.present = i;

		if (indices.isComplete())
			break;

		++i;
	}

	return indices;
}

}
