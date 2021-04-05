#pragma once

class Octree
{

	struct Node
	{
		Primitives::BoxUniform box;
		Node* children[8] = { nullptr };
		std::list<Mesh<PosVertex>> objects;
	};


public:
	void Create(glm::vec3 position, float halfExtent, int stopDepth, Device& owner)
	{
		m_owner = &owner;
		//head = CreateTree(position, halfExtent, stopDepth);
		auto& view = ECS::Get().view<TransformComponent, 
			DeferredRenderComponent>();

		view.each([this, stopDepth, position, halfExtent](const entt::entity entity, 
						 const TransformComponent& transform,
						 DeferredRenderComponent& render)
		{
			auto& mesh = render.mesh;
			// Convert from standard vertex mesh to position only mesh
			auto vertices = mesh.GetVertexBufferDataCopy<glm::vec3>(0);
			auto indices = mesh.GetIndexBufferDataCopy();
			int vertexCount = vertices.size();
			int indexCount = indices.size();

			Mesh<PosVertex>::Data data;
			// Copy data over
			data.vertices.resize(vertexCount);
			std::memcpy(data.vertices.data(), vertices.data(), sizeof(glm::vec3) * vertexCount);
			data.indices.resize(indexCount);
			std::memcpy(data.indices.data(), indices.data(), sizeof(uint32_t) * indexCount);

			const glm::mat4 model = transform.model;

			// Turn into world space
			for (auto& vertex : data.vertices)
				vertex.pos = static_cast<glm::vec3>(model * glm::vec4(vertex.pos,1.0f));

			head = new Node();
			head->box.position = position;
			head->box.halfExtent = halfExtent;

			InsertObject(*head, data, position, halfExtent, stopDepth);
		});
	}

	void RenderCells(vk::CommandBuffer commandBuffer, 
				vk::PipelineLayout pipelineLayout)
	{
		//ASSERT(head != nullptr, "Attempting to render octree without creating it");
		if (head == nullptr) return;
		Mesh<PosVertex>::CubeList.Bind(commandBuffer);
		RenderCellsRecursively(commandBuffer, pipelineLayout, head);
	}

	void RenderObjects(vk::CommandBuffer commandBuffer,
				vk::PipelineLayout pipelineLayout)
	{
		//ASSERT(head != nullptr, "Attempting to render octree without creating it");
		if (head == nullptr) return;
		srand(1305871305);
		utils::PushIdentityModel(commandBuffer, pipelineLayout);
		RenderObjectsRecursively(commandBuffer, pipelineLayout, head, 0);
	}


	void Destroy()
	{
		m_owner->waitIdle();
		DestroyRecursively(head);
		head = nullptr;
	}

	bool IsInitialized()
	{
		return head != nullptr;
	}

	inline static uint32_t MinimumTriangles = 500;

private:
	bool RenderCellsRecursively(vk::CommandBuffer commandBuffer,
						   vk::PipelineLayout pipelineLayout,
						   Node* node)
	{
		bool childExists = false;
		for (int i = 0; i < 8; ++i)
		{
			Node* child = node->children[i];
			if (child != nullptr)
				childExists |= RenderCellsRecursively(commandBuffer, pipelineLayout, child);
		}
		if (node->objects.empty() && childExists == false)
		{
			return false;
		}

		glm::mat4 model = glm::translate(utils::identity, node->box.position);
		model = glm::scale(model, glm::vec3(node->box.halfExtent * 2.0f));

		commandBuffer.pushConstants(
			pipelineLayout,
			vk::ShaderStageFlagBits::eVertex,
			0, sizeof(glm::mat4), &model
		);
		Mesh<PosVertex>::CubeList.Draw(commandBuffer);
		return true;
	}

	void RenderObjectsRecursively(vk::CommandBuffer commandBuffer,
						   vk::PipelineLayout pipelineLayout,
						   Node* node, int index)
	{
		if (node == nullptr) return;

		for (int i = 0; i < 8; ++i)
		{
			RenderObjectsRecursively(commandBuffer,
									 pipelineLayout,
									 node->children[i], index * 8 + i);
		}

		for (auto& obj : node->objects)
		{
			obj.Bind(commandBuffer);
			
			glm::vec3 color = { utils::Random(), utils::Random(), utils::Random() };

			commandBuffer.pushConstants(
				pipelineLayout,
				vk::ShaderStageFlagBits::eFragment,
				sizeof(glm::mat4), sizeof(utils::UBOColor), &color
			);

			//TransformComponent transform = ECS::Get().get<TransformComponent>(obj.entity);
			//transform.PushModel(commandBuffer, pipelineLayout);
			obj.Draw(commandBuffer);
		}
	}


	void DestroyRecursively(Node* node)
	{
		for (int i = 0; i < 8; ++i)
		{
			Node* child = node->children[i];
			if (child != nullptr)
				DestroyRecursively(child);
		}
		for (auto& obj : node->objects)
			obj.Destroy();
		delete node;
	}


	void InsertObject(Node& node, 
					  Mesh<PosVertex>::Data& data,
					  const glm::vec3& cellPosition,
					  float cellHalfExtent, int stopDepth)
	{
		int triangleCount = data.indices.size() / 3;
		if (triangleCount < MinimumTriangles || stopDepth == 0)
		{
			node.objects.emplace_back();
			Mesh<PosVertex>& obj = node.objects.back();
			obj.CreateStatic(data.vertices, data.indices, *m_owner);
			return;
		}

		int index = 0;
		static Primitives::Plane planes[3] = {
			{ { 1.0f, 0.0f, 0.0f } },
			{ { 0.0f, 1.0f, 0.0f } },
			{ { 0.0f, 0.0f, 1.0f } }
		};

		struct SplitData
		{
			SplitData(Mesh<PosVertex>::Data&& data, int mask)
				: data(data), mask(mask) {}
			Mesh<PosVertex>::Data data = {};
			int mask = 0;
		};
		std::vector<SplitData> splits;
		splits.emplace_back(std::move(data), 0);

		for (int i = 0; i < 3; ++i)
		{
			// Assign plane data
			planes[i].position = node.box.position;
			planes[i].D = glm::dot(node.box.position, planes[i].normal);

			int splitSize = splits.size();
			// For each set of vertices placed in the split vector
			for (int j = 0; j < splitSize; ++j)
			{
				auto& split = splits[j];
				int straddling = Mesh<PosVertex>::IsStraddlingPlane(
					split.data.vertices.data(),
					split.data.vertices.size(),
					planes[i]);
				// 0 means a straddle, 1 is all positive, -1 is all negative
				if (straddling != 0)
				{
					// If not straddling and on back side,  mask if positive
					if (straddling == 1)
						split.mask |= (1 << i);
				}
				else
				{
					Mesh<PosVertex>::Data front, back;
					// Straddling, split into two (front, back pair)
					Mesh<PosVertex>::Clip(split.data, planes[i], front, back);
					int mask = split.mask;
					// Erase this SplitData as we have just split it into two
					splits.erase(splits.begin() + j--);
					--splitSize;
					// Emplace front and back-side split, front is masked by the current plane
					if (!front.indices.empty())
						splits.emplace_back( std::move(front), mask | (1 << i) );
					if (!back.indices.empty())
						splits.emplace_back( std::move(back), mask );
				}
			}
		}

		int splitSize = splits.size();

		float step = cellHalfExtent * 0.5f;
		glm::vec3 offset;
		for (int i = 0; i < splitSize; ++i)
		{
			auto& split = splits[i];
			offset.x = ((split.mask & 1) ? step : -step);
			offset.y = ((split.mask & 2) ? step : -step);
			offset.z = ((split.mask & 4) ? step : -step);

			auto* child = node.children[split.mask];
			if (child == nullptr)
			{
				child = node.children[split.mask] = new Node();
				child->box.halfExtent = step;
				child->box.position = cellPosition + offset;
			}
			

			// Insert into children based on index mask
			InsertObject(*child, split.data, cellPosition + offset, step, stopDepth - 1);
		}
	}


	Node* head = nullptr;
	Device* m_owner;
};