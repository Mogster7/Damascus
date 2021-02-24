#pragma once
#include "Overlay/EditorBlock.h"

class EditorWindow
{
public:
	explicit EditorWindow(const std::string& title, bool removeOnSceneSwitch = false);

	virtual ~EditorWindow();

	virtual void Update(float dt);

	void AddBlock(EditorBlock* editorBlock);

	std::vector<EditorBlock*>& GetBlocks();

	std::string& GetTitle();

	bool& GetShowWindow();

	void SetTitle(const std::string& title)
	{
		m_title = title;
	}

	bool removeOnSceneSwitch = false;

private:
	std::string m_title;
	std::vector<EditorBlock*> m_blocks;
	bool m_showWindow;
};