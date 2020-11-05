//------------------------------------------------------------------------------
//
// File Name:	PhysicalDevice.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/18/2020
//
//------------------------------------------------------------------------------
#include "PhysicalDevice.h"
#include "Renderer.h"
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

    vk::SurfaceKHR surface = Renderer::GetInstance().GetSurface();
    if (device.getSurfaceFormatsKHR(surface).empty() ||
        device.getSurfacePresentModesKHR(surface).empty())
        return false;
        

    return true;
}


PhysicalDevice::PhysicalDevice(const vk::PhysicalDevice& other) : vk::PhysicalDevice(other) { }

void PhysicalDevice::Create()
{
    std::vector<vk::PhysicalDevice> devices = Renderer::GetInstance().enumeratePhysicalDevices();
    if (devices.empty()) throw std::runtime_error("Failed to find GPU with Vulkan support");

    auto it = std::find_if(devices.begin(), devices.end(), [this](const auto& device) -> bool
        {
            return IsDeviceSuitable(device);
        });

    if (it == devices.end())
        throw std::runtime_error("Failed to find a suitable GPU");

    (*this) = PhysicalDevice(*it);
    m_QueueFamilyIndices = FindQueueFamilies(*this);
    getProperties(&m_Properties);
}

void PhysicalDevice::CreateLogicalDevice(Device& device)
{
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> queueFamilies = { m_QueueFamilyIndices.graphics.value(), m_QueueFamilyIndices.present.value() };

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
    return m_QueueFamilyIndices;
}

vk::DeviceSize PhysicalDevice::GetMinimumUniformBufferOffset() const
{
    return m_Properties.limits.minUniformBufferOffsetAlignment;
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

        if (pd.getSurfaceSupportKHR(i, Renderer::GetInstance().GetSurface()))
            indices.present = i;

        if (indices.isComplete()) break;

        ++i;
    }

    return indices;
}
