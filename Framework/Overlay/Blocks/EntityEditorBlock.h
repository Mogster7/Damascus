#pragma once
#include "Overlay/EditorBlock.h"
#include "Object/Object.h"

class EntityEditorBlock : public EditorBlock
{
public:
	EntityEditorBlock(std::vector<entt::entity>& entities) : entities(entities) {}
	~EntityEditorBlock();
	void UpdateTransform(entt::registry& reg, entt::entity entity);
	void Update(float dt) override;

	std::vector<entt::entity>& entities;
};
