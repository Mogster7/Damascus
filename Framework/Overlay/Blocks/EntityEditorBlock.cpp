#include "EntityEditorBlock.h"

EntityEditorBlock::~EntityEditorBlock()
{

}


void EntityEditorBlock::UpdateTransform(entt::registry& reg, entt::entity entity)
{
	auto transform = reg.try_get<TransformComponent>(entity);
	if (transform == nullptr) return;

	ImGui::InputFloat3(std::string("Position##Entity").c_str(), &transform->m_position[0]);
	ImGui::InputFloat3(std::string("Scale##Entity").c_str(), &transform->m_scale[0]);
	ImGui::SliderFloat3(std::string("Rotation##Entity").c_str(), &transform->m_rotation[0], 0.0f, 720.0f);
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
