#pragma once

class RenderComponentSystem : public ComponentSystem
{
public:
	void Update(float dt) override {};

	template <typename ComponentType>
	void RenderEntities(vk::CommandBuffer commandBuffer,
						vk::DescriptorSet descriptorSet,
						vk::PipelineLayout pipelineLayout)
	{
		//// Bind descriptor sets
		commandBuffer.bindDescriptorSets(
			// Point of pipeline and layout
			vk::PipelineBindPoint::eGraphics,
			pipelineLayout,
			0,
			1,
			&descriptorSet, // 1 to 1 with command buffers
			0,
			nullptr
		);

		auto& registry = ECS::Get();
		auto view = registry.view<TransformComponent, ComponentType>();

		view.each(
			[=](const TransformComponent& transform, const ComponentType& render)
		{
			render.mesh->Bind(commandBuffer);
			transform.PushModel(commandBuffer, pipelineLayout);
			render.mesh->Draw(commandBuffer);
		});
	}

	//template <>
	//void RenderEntities<DeferredRenderComponent>(vk::CommandBuffer commandBuffer,
	//											 vk::DescriptorSet descriptorSet,
	//											 vk::PipelineLayout pipelineLayout)
	//{
	//	// Take care of this in the demo scene for now
	//}



};