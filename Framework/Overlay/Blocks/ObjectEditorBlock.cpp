#include "ObjectEditorBlock.h"

ObjectEditorBlock::~ObjectEditorBlock()
{

}

void ObjectEditorBlock::Update(float dt)
{

	int i = 0;
	for (auto& object : sceneObjects)
	{
		if (ImGui::TreeNode(std::string("Object: " + std::to_string(i++)).c_str()))
		{
			ImGui::SliderFloat3(std::string("Position" + std::to_string(i++)).c_str(), &object.m_position[0], -10.0f, 10.0f);
			ImGui::SliderFloat3(std::string("Scale" + std::to_string(i++)).c_str(), &object.m_scale[0], 0.1f, 3.0f);
			ImGui::SliderFloat3(std::string("Rotation" + std::to_string(i++)).c_str(), &object.m_rotation[0], 0.0f, 720.0f);
			object.dirty = true;

			ImGui::TreePop();
		}
	}

}
