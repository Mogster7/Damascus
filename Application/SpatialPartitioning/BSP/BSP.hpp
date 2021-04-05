#pragma once
class BSP
{

public:
	struct Node
	{
		Node* left = nullptr;
		Node* right = nullptr;
		std::list<Mesh<PosVertex>> objects;
	};

	void Create(Device& owner)
	{
		m_owner = &owner;
		auto& view = ECS::Get().view<TransformComponent,
			DeferredRenderComponent>();

		std::vector<Mesh<PosVertex>::Data> meshData;
		meshData.resize(view.size_hint());

		int i = 0;
		view.each([this, &meshData, &i](const entt::entity entity, 
						 const TransformComponent& transform,
						 DeferredRenderComponent& render)
		{
			auto& mesh = render.mesh;
			const glm::mat4 model = transform.model;

			auto verts = render.mesh.GetVertexBufferDataCopy<glm::vec3>(0);
			auto indices = render.mesh.GetIndexBufferDataCopy();

			// Turn into world space
			for (auto& vertex : verts)
				vertex = static_cast<glm::vec3>(model * glm::vec4(vertex,1.0f));

			auto vertexCount = verts.size();
			meshData[i].vertices = std::move(reinterpret_cast<std::vector<PosVertex>&>(verts));
			meshData[i].indices = std::move(indices);
			++i;
		});

	

	}

	void RenderObjects(vk::CommandBuffer commandBuffer,
				vk::PipelineLayout pipelineLayout)
	{
		//ASSERT(head != nullptr, "Attempting to render octree without creating it");
		if (head == nullptr) return;
		srand(1305871305);
		utils::PushIdentityModel(commandBuffer, pipelineLayout);
	}


	void Destroy()
	{
		m_owner->waitIdle();
		head = nullptr;
	}

	bool IsInitialized()
	{
		return head != nullptr;
	}

	inline static uint32_t MinimumTriangles = 500;

private:
	Node* Build(std::vector<Mesh<PosVertex>::Data>& data, int depth)
	{
		if (depth == 0)
			return nullptr;
			
		int numMeshes = data.size();

		Primitives::Plane splitPlane;
		
		std::vector<Mesh<PosVertex>::Data> frontList, backList;
		
		//for(int i = 0; i < numMeshes)
	}

	Node* head;
	Device* m_owner;
};

