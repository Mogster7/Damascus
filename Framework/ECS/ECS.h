#pragma once

class TransformComponentSystem;
class RenderComponentSystem;

#define COMPONENTS(x)\
	TransformComponent##x, RenderComponent##x\


#define COMPONENT_SYSTEMS COMPONENTS(System)

class ECS
{
public:
	static entt::registry& Get() {
		ASSERT(registry != nullptr, "ECS not initialized");
		return *registry;
	}
	

	static void CreateSystems();

	template <class System>
	static System& GetSystem() { return std::get<System>(systems); }

	static void UpdateSystems(float dt);
	static void DestroySystems();

private:
	static std::tuple<
		COMPONENT_SYSTEMS
	> systems;
	inline static entt::registry* registry = nullptr;
};


