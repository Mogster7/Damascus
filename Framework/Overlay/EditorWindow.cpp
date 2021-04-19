
#include "EditorWindow.h"

EditorWindow::EditorWindow(const std::string& title, bool removeOnSceneSwitch /*= false*/) :
	m_title(title)
{
}

void EditorWindow::AddBlock(EditorBlock* editorBlock)
{
	m_blocks.emplace_back(editorBlock);
}

void EditorWindow::Update(float dt)
{
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
	if (m_showWindow)
	{
		ImGui::Begin(m_title.c_str(), &m_showWindow, window_flags);
		for (auto block : m_blocks)
		{
			for (auto& cb : block->updateCallbacks)
				cb();
			block->Update(dt);
		}
		ImGui::End();
	}
}

EditorWindow::~EditorWindow()
{
	for (auto& block : m_blocks) delete block;
}

std::vector<EditorBlock*>& EditorWindow::GetBlocks()
{
	return m_blocks;
}

std::string& EditorWindow::GetTitle()
{
	return m_title;
}

bool& EditorWindow::GetShowWindow()
{
	return m_showWindow;
}
