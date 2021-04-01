#include "EntityEditorBlock.h"

EntityEditorBlock::~EntityEditorBlock()
{

}


void EntityEditorBlock::UpdateTransform(entt::registry& reg, entt::entity entity)
{
	auto transform = reg.try_get<TransformComponent>(entity);
	if (transform == nullptr) return;

	ImGui::SliderFloat3(std::string("Position").c_str(), &transform->m_position[0], -10.0f, 10.0f);
	ImGui::SliderFloat3(std::string("Scale").c_str(), &transform->m_scale[0], 0.1f, 3.0f);
	ImGui::SliderFloat3(std::string("Rotation").c_str(), &transform->m_rotation[0], 0.0f, 720.0f);
	transform->dirty = true;
}

void EntityEditorBlock::Update(float dt)
{
	auto& reg = ECS::Get();
	int i = 0;
	reg.each([&](auto entity)
	{
		if (ImGui::TreeNode(std::string("Entity " + std::to_string(i++)).c_str()))
		{
			UpdateTransform(reg, entity);
			ImGui::TreePop();
		}
	});

}
