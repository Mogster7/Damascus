#pragma once
#include "Overlay/EditorBlock.h"

class StatsEditorBlock : public EditorBlock
{
public:
	StatsEditorBlock();
	~StatsEditorBlock();
	void Update(float dt) override;
};