#pragma once
#include "Overlay/EditorWindow.h"
#include "Overlay/EditorBlock.h"

class Overlay
{
public:
	void Create(std::weak_ptr<Window> window, 
				Instance& instance,
				PhysicalDevice& physicalDevice,
				Device& device);
	void Destroy();

	void Begin();

	void Update(float dt)
	{
		for (auto& window : m_editorWindows) 
			window->Update(dt);
	}

	void RecordCommandBuffers(uint32_t imageIndex);

	void PushEditorWindow(EditorWindow* editorWindow)
	{
		m_editorWindows.emplace_back(editorWindow);
	}
	

    DescriptorPool descriptorPool;
	RenderPass renderPass;
	CommandPool commandPool;
	std::vector<vk::CommandBuffer> commandBuffers;
	std::vector<FrameBuffer> framebuffers;

private:
	Device* m_owner;
	std::vector<EditorWindow*> m_editorWindows;

};
