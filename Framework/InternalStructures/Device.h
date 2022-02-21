//------------------------------------------------------------------------------
//
// File Name:	Device.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/18/2020 
//
//------------------------------------------------------------------------------
#pragma once
namespace dm
{
class Swapchain;
class Descriptors;
class DescriptorPool;

class Device : public IVulkanType<vk::Device>, public IOwned<PhysicalDevice>
{
public:
DM_TYPE_VULKAN_OWNED_BODY(Device, IOwned<PhysicalDevice>)

	void Create(const vk::DeviceCreateInfo& createInfo, PhysicalDevice* owner);
    void Destroy();

	~Device() noexcept override;

	void Update(float dt);
    [[nodiscard]] int ImageCount() const;
    [[nodiscard]] Swapchain& Swapchain();
    [[nodiscard]] Descriptors& Descriptors();
    [[nodiscard]] DescriptorPool& DescriptorPool();
    [[nodiscard]] int ImageIndex() const;

    // Kept freeing behavior for descriptor sets, if we need it in the future
//    void FreeDescriptorSet(vk::DescriptorSet set)
//    {
//        setsToFree[ImageIndex()][0].emplace_back(set);
//    }
//
//    void FreeDescriptorSets(std::vector<vk::DescriptorSet>&& sets)
//    {
//
//    }
    //std::vector<std::vector<std::vector<vk::DescriptorSet>>> setsToFree;

	VmaAllocator allocator = {};
	vk::Queue graphicsQueue = {};
	vk::Queue presentQueue = {};

private:
	void CreateAllocator();
};

template <class T>
using ImageAsync = std::vector<T>;

template <class T>
using FrameAsync = std::array<T, MAX_FRAME_DRAWS>;
}
