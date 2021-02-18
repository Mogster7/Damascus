//------------------------------------------------------------------------------
//
// File Name:	PhysicalDevice.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/18/2020
//
//------------------------------------------------------------------------------
#include "PhysicalDevice.h"
#include "RenderingContext.h"
#include <set>
#include <vector>

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
    auto indices = FindQueueFamilies(device);
    if (!indices.isComplete()) return false;

    bool extensionsSupported = CheckDeviceExtensionSupport(device);
    if (!extensionsSupported) return false;

    vk::SurfaceKHR surface = RenderingContext::Get().instance.GetSurface();
    if (device.getSurfaceFormatsKHR(surface).empty() ||
        device.getSurfacePresentModesKHR(surface).empty())
        return false;
        

    return true;
}


PhysicalDevice::PhysicalDevice(const vk::PhysicalDevice& other) : vk::PhysicalDevice(other) { }

void PhysicalDevice::Create(Instance& instance)
{
    std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
    if (devices.empty()) throw std::runtime_error("Failed to find GPU with Vulkan support");

    auto it = std::find_if(devices.begin(), devices.end(), [this](const auto& device) -> bool
        {
            return IsDeviceSuitable(device);
        });

    if (it == devices.end())
        throw std::runtime_error("Failed to find a suitable GPU");

    (*this) = PhysicalDevice(*it);
	this->instance = &instance;
    queueFamilyIndices = FindQueueFamilies(*this);
    getProperties(&properties);
}

void PhysicalDevice::CreateLogicalDevice(Device& device)
{
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> queueFamilies = { queueFamilyIndices.graphics.value(), queueFamilyIndices.present.value() };

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
    
    device.Create(&device, createInfo, *this);
}


const QueueFamilyIndices& PhysicalDevice::GetQueueFamilyIndices() const
{
    return queueFamilyIndices;
}

vk::DeviceSize PhysicalDevice::GetMinimumUniformBufferOffset() const
{
    return properties.limits.minUniformBufferOffsetAlignment;
}

QueueFamilyIndices PhysicalDevice::FindQueueFamilies(vk::PhysicalDevice pd)
{
    QueueFamilyIndices indices;

    std::vector<vk::QueueFamilyProperties> queueFamilies = pd.getQueueFamilyProperties();

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
            indices.graphics = i;

        if (pd.getSurfaceSupportKHR(i, RenderingContext::Get().instance.GetSurface()))
            indices.present = i;

        if (indices.isComplete()) break;

        ++i;
    }

    return indices;
}
