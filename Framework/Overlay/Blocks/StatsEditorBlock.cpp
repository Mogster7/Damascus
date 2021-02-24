#include "StatsEditorBlock.h"

StatsEditorBlock::StatsEditorBlock()
{

}

StatsEditorBlock::~StatsEditorBlock()
{

}

void StatsEditorBlock::Update(float dt)
{
	auto io = ImGui::GetIO();
	ImGui::Text("ImGui FPS: %f", 1000.0f / io.Framerate);
	ImGui::Text("FPS: %f", io.Framerate);
	ImGui::Text("Display X:%d", static_cast<int>(io.DisplaySize.x));
	ImGui::Text("Display Y:%d", static_cast<int>(io.DisplaySize.y));
}
