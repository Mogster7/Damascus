//------------------------------------------------------------------------------
//
// File Name:	PhysicalDevice.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/18/2020 
//
//------------------------------------------------------------------------------
#pragma once

namespace vk
{
    PhysicalDevice;
    Device;
}

class QueueFamilyIndices
{
public:
    eastl::optional<uint32_t> graphics;
    eastl::optional<uint32_t> present;

    bool isComplete() const
    {
        return graphics.has_value() & present.has_value();
    }
};

class Device;

class PhysicalDevice : public eastl::safe_object, public vk::PhysicalDevice
{
public:
    PhysicalDevice() = default;
    PhysicalDevice(const vk::PhysicalDevice& other);
    ~PhysicalDevice() = default;

    void Create();
    void CreateLogicalDevice(Device& device);
    const QueueFamilyIndices& GetQueueFamilyIndices() const;
    vk::PhysicalDeviceProperties GetProperties() const { return m_Properties; }
    vk::DeviceSize GetMinimumUniformBufferOffset() const;





private:
    static QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice pd);
    bool IsDeviceSuitable(vk::PhysicalDevice) const;
    bool CheckDeviceExtensionSupport(vk::PhysicalDevice) const;

    QueueFamilyIndices m_QueueFamilyIndices;
    vk::PhysicalDeviceProperties m_Properties;
};



