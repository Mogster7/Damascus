#pragma once

class PhysicsComponentSystem : public ComponentSystem
{
public:
	void Update(float dt) override
	{
		auto& reg = ECS::Get();
		auto& view = reg.view<PhysicsComponent, TransformComponent>();
		view.each([dt](PhysicsComponent& physics, TransformComponent& transform)
		{
			transform.SetPosition(transform.GetPosition() + physics.velocity * dt);
		});
	}
};
