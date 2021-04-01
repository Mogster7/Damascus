
std::tuple<
	COMPONENT_SYSTEMS
> ECS::systems;


void ECS::CreateSystems()
{
	registry = new entt::registry;
	std::apply([](auto& ...x) { (..., x.Create()); }, systems);
}

void ECS::UpdateSystems(float dt)
{
	std::apply([dt](auto& ...x) { (..., x.Update(dt)); }, systems);
}

void ECS::DestroySystems()
{
	std::apply([](auto& ...x) { (..., x.Destroy()); }, systems);
	delete registry;
}
