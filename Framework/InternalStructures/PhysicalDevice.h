//------------------------------------------------------------------------------
//
// File Name:	PhysicalDevice.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/18/2020 
//
//------------------------------------------------------------------------------
#pragma once

namespace bk {

class QueueFamilyIndices
{
public:
	std::optional<uint32_t> graphics;
	std::optional<uint32_t> present;

	[[nodiscard]] bool isComplete() const
	{
		return graphics.has_value() && present.has_value();
	}
};

class Device;
class RenderingContext;

class PhysicalDevice : public IVulkanType<vk::PhysicalDevice>, public IOwned<RenderingContext>
{
public:
BK_TYPE_VULKAN_OWNED_BODY(PhysicalDevice, IOwned<RenderingContext>)

	void Create(RenderingContext* owner);
	~PhysicalDevice() = default;

	[[nodiscard]] const QueueFamilyIndices& GetQueueFamilyIndices() const;

	[[nodiscard]] vk::PhysicalDeviceProperties GetProperties() const
	{
		return properties;
	}

	vk::DeviceSize GetMinimumUniformBufferOffset() const;

	QueueFamilyIndices queueFamilyIndices;
	vk::PhysicalDeviceProperties properties;

private:
	static QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice pd);

	[[nodiscard]] bool IsDeviceSuitable(vk::PhysicalDevice) const;

	[[nodiscard]] bool CheckDeviceExtensionSupport(vk::PhysicalDevice) const;
};

}