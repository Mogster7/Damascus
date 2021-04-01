#pragma once

class TransformComponentSystem : public ComponentSystem
{
public:
	void Update(float dt) override
	{
		auto& reg = ECS::Get();
		auto& view = reg.view<TransformComponent>();
		view.each([](TransformComponent& transform)
		{
			if (transform.dirty) transform.UpdateModel();
		});
	}
};
