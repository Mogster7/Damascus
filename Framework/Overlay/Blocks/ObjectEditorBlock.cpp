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

			auto position = object.GetPosition();
			ImGui::InputFloat3("Position", &position[0]);
			object.SetPosition(position);

			auto scale = object.GetScale();
			ImGui::InputFloat3("Scale", &scale[0]);
			object.SetScale(scale);

			auto rotation = object.GetRotation();
			ImGui::InputFloat3("Rotation", &rotation[0]);
			object.SetRotation(rotation);

			ImGui::TreePop();
		}
	}

}
