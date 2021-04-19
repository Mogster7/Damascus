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

	void Create(int depth, Device& owner)
	{
		m_owner = &owner;
		auto& view = ECS::Get().view<TransformComponent,
			DeferredRenderComponent>();

		std::vector<Mesh<PosVertex>::Data> meshData;
		meshData.resize(view.size_hint());

		int i = 0;
		view.each([this, depth, &meshData, &i](const entt::entity entity, 
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

		srand(timer * 10.0f);
		head = Build(meshData);
	}

	void RenderObjects(vk::CommandBuffer commandBuffer,
				vk::PipelineLayout pipelineLayout)
	{
		//ASSERT(head != nullptr, "Attempting to render octree without creating it");
		if (head == nullptr) return;
		srand(1305871305);
		utils::PushIdentityModel(commandBuffer, pipelineLayout);
		RenderRecursively(head, commandBuffer, pipelineLayout);
	}

	void Update(float dt)
	{
		timer += dt;
	}

	void Destroy()
	{
		ASSERT(head != nullptr, "Attempting to destroy BSP tree without creating it");
		m_owner->waitIdle();
		DestroyRecursively(head);
		head = nullptr;
	}

	bool IsInitialized()
	{
		return head != nullptr;
	}

	inline static uint32_t MinimumTriangles = 500;
	inline static float SplitBlend = 0.8f;
	inline static uint32_t PlaneSamples = 5;
	inline static float PlaneAreaTestScale = 5.0f;

private:
	void RenderRecursively(Node* node,
								  vk::CommandBuffer commandBuffer,
								  vk::PipelineLayout layout)
	{
		if (node->left)
			RenderRecursively(node->left, commandBuffer, layout);
		if (node->right)
			RenderRecursively(node->right, commandBuffer, layout);

		for (auto& obj : node->objects)
		{
			obj.Bind(commandBuffer);
			
			glm::vec3 color = { utils::Random(), utils::Random(), utils::Random() };

			commandBuffer.pushConstants(
				layout,
				vk::ShaderStageFlagBits::eFragment,
				sizeof(glm::mat4), sizeof(utils::UBOColor), &color
			);

			obj.Draw(commandBuffer);
		}

	}

	void DestroyRecursively(Node* node)
	{
		if (node->left)
			DestroyRecursively(node->left);
		if (node->right)
			DestroyRecursively(node->right);

		for (auto& obj : node->objects)
			obj.Destroy();
		delete node;
	}


	Primitives::Plane FindSplittingPlane(Mesh<PosVertex>::Data& d)
	{
		Primitives::Plane bestPlane;
		float bestScore = std::numeric_limits<float>::max();

		int numVertices = d.vertices.size();


		for (int j = 0; j < PlaneSamples; ++j)
		{
			int numFront = 0, numBack = 0, numStraddling = 0;
			glm::vec3 randomPoints[2];
			randomPoints[0] = d.vertices[utils::RandomInt(0, numVertices)].pos;
			randomPoints[1] = d.vertices[utils::RandomInt(0, numVertices)].pos;

			Primitives::Plane plane = Primitives::GeneratePlaneBetweenTwoPoints(randomPoints[0], randomPoints[1]);

			int straddling = Mesh<PosVertex>::IsStraddlingPlane(d.vertices.data(), d.vertices.size(), plane);
			switch (straddling)
			{
			case -1:
				++numBack;
				break;
			case 0:
				++numStraddling;
				break;
			case 1:
				++numFront;
				break;
			default:
				ASSERT(false, "Invalid case when splitting BSP by plane");
			}

			float score = SplitBlend * numStraddling + (1.0f - SplitBlend) * std::abs(numFront - numBack);
			if (score < bestScore)
			{
				bestPlane = plane;
				bestScore = score;
			}

		}

		return bestPlane;
	}

	Node* Build(std::vector<Mesh<PosVertex>::Data>& data)
	{
		Node* node = new Node();
		int numMeshes = data.size();

		
		std::vector<Mesh<PosVertex>::Data> frontList, backList;
		auto EmplaceObject =
			[this, node](Mesh<PosVertex>::Data& d)
		{
			node->objects.emplace_back();
			auto& obj = node->objects.back();
			obj.CreateStatic(d.vertices, d.indices, *m_owner);
		};
		
		for (int i = 0; i < numMeshes; ++i)
		{
			auto& d = data[i];
			uint32_t triangleCount = d.indices.size() / 3;
			if (triangleCount < MinimumTriangles)
			{
				EmplaceObject(d);
				continue;
			}

			Primitives::Plane splitPlane = FindSplittingPlane(d);
			int straddle = Mesh<PosVertex>::IsStraddlingPlane(d.vertices.data(), d.vertices.size(), splitPlane);
			if (straddle != 0)
			{
				frontList.emplace_back(d);
			}
			else
			{
				Mesh<PosVertex>::Data front, back;
				Mesh<PosVertex>::Clip(d, splitPlane, front, back);

				int frontTriangleCount = front.indices.size() / 3;
				int backTriangleCount = back.indices.size() / 3;
				// BAIL if we're at the point where splitting increases
				// our triangle count
				if (frontTriangleCount >= triangleCount ||
					backTriangleCount >= triangleCount)
				{
					EmplaceObject(d);
					continue;
				}


				frontList.emplace_back(front);
				backList.emplace_back(back);
			}
		}

		if (frontList.size() > 0)
			node->left = Build(frontList);
		if (backList.size() > 0)
			node->right = Build(backList);
		return node;
	}

	Node* head;
	Device* m_owner;
	float timer = 0.0f;
};

