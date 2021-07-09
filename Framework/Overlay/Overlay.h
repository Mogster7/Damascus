#pragma once

#include "Overlay/EditorWindow.h"
#include "Overlay/EditorBlock.h"

namespace bk {

//BK_TYPE(Overlay)
class Overlay : public IOwned<Device>
{
public:
	BK_TYPE_OWNED_BODY(Overlay, IOwned<Device>)

	void Create(std::weak_ptr<Window> window, Device* owner);

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
	std::vector<EditorWindow*> m_editorWindows;
};

}