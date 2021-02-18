#pragma once

class Overlay
{
public:
	void Create(std::weak_ptr<Window> window, 
				Instance& instance,
				PhysicalDevice& physicalDevice,
				Device& device);
	void Destroy();

	void RecordCommandBuffers(uint32_t imageIndex);

    DescriptorPool descriptorPool;
	RenderPass renderPass;
	CommandPool commandPool;
	std::vector<vk::CommandBuffer> commandBuffers;
	std::vector<FrameBuffer> framebuffers;

private:
	Device* m_owner;

};
