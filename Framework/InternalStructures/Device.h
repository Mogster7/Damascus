//------------------------------------------------------------------------------
//
// File Name:	Device.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/18/2020 
//
//------------------------------------------------------------------------------
#pragma once
namespace bk {

class Device : public IVulkanType<vk::Device>, public IOwned<PhysicalDevice>
{
public:
BK_TYPE_VULKAN_OWNED_BODY(Device, IOwned<PhysicalDevice>)

	void Create(const vk::DeviceCreateInfo& createInfo, PhysicalDevice* owner);

	void Update(float dt);

	// Return whether or not the surface is out of date
	bool PrepareFrame(const uint32_t frameIndex);

	bool SubmitFrame(const uint32_t frameIndex, const vk::Semaphore* wait, uint32_t waitCount);

	void DrawFrame(const uint32_t frameIndex);

	VmaAllocator allocator = {};
	vk::Queue graphicsQueue = {};
	vk::Queue presentQueue = {};

private:

	void CreateAllocator();

	void CreateCommandPool();

	void CreateCommandBuffers(bool recreate = false);

	void CreateSync();

	void RecordCommandBuffers(uint32_t imageIndex);

	void CreateUniformBuffers();

	void CreateDescriptorPool();

	void CreateDescriptorSets();

	void UpdateUniformBuffers(uint32_t imageIndex);

	void UpdateModel(glm::mat4& newModel);

	void RecreateSurface();
};

}