#pragma once
#include "Primitives/Primitives.hpp"

class Octree
{
	struct Object
	{
		entt::entity entity;
		Object* next = nullptr;
	};

	struct Node
	{
		BoxUniform box;
		Node* children[8] = { nullptr };
		entt::entity* entities = nullptr;
	};

public:
	void Create(glm::vec3 position, float halfExtent, int stopDepth)
	{
		head = CreateTree(position, halfExtent, stopDepth);
		auto& view = ECS::Get().view<TransformComponent>();
		//view.each([](const entt::entity entity, const TransformComponent& transform)
		//{
		//	if ()
		//})
	}


private:
	Node* CreateTree(glm::vec3 position, float halfExtent, int stopDepth)
	{
		if (stopDepth < 0) return nullptr;

		float step = halfExtent * 0.5f;
			
		Node* node = new Node();
		node->box.position = position;
		node->box.halfExtent = halfExtent;
		glm::vec3 offset;
		for (int i = 0; i < 8; ++i)
		{
			offset.x = ((i & 1) ? step : -step);
			offset.y = ((i & 2) ? step : -step);
			offset.z = ((i & 4) ? step : -step);
			node->children[i] = CreateTree(position + offset, halfExtent, stopDepth - 1);
		}
		return node;
	}
	Node* head;
};