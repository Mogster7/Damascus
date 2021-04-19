#pragma once

class Octree
{

	struct Node
	{
		Primitives::BoxUniform box;
		Node* children[8] = { nullptr };
		struct Object
		{
			Mesh<PosVertex> mesh;
			Primitives::Box bb;
			bool colliding = false;
		};
		std::list<Object> objects = { };
		glm::vec3 color;
		bool leaf = false;
	};


public:
	void Create(glm::vec3 position, float halfExtent, int stopDepth, Device& owner)
	{
		m_owner = &owner;
		//head = CreateTree(position, halfExtent, stopDepth);
		auto& view = ECS::Get().view<TransformComponent, 
			DeferredRenderComponent>();

		head = new Node();
		head->box.position = position;
		head->box.halfExtent = halfExtent;
		head->color = glm::vec3(utils::Random(), utils::Random(), utils::Random());


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

			InsertObject(*head, data, position, halfExtent);
		});
	}

	void RenderCells(vk::CommandBuffer commandBuffer, 
				vk::PipelineLayout pipelineLayout)
	{
		if (head == nullptr) return;
		Mesh<PosVertex>::CubeList.Bind(commandBuffer);
		RenderCellsRecursively(commandBuffer, pipelineLayout, head);
	}

	void RenderObjects(vk::CommandBuffer commandBuffer,
				vk::PipelineLayout pipelineLayout)
	{
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

	bool CollisionTest(const Primitives::Box& collider, 
					   glm::mat4& transform)
	{
		if (!head) return false;
		colliderTransform = &transform;
		return DetectCollisionBroad(collider, head);
	}

private:

	// GJK
	bool DetectCollisionNarrow(const Primitives::Box& collider,
							   const Node::Object& object)
	{
		static const glm::vec3 origin = glm::vec3(0.0f);

		const Primitives::Box& objBox = object.bb;
		auto MinkowskiSupport =
			[this](const Primitives::Box& collider,
			   const Primitives::Box& objBox,
			   const glm::vec3& d) -> glm::vec3
		{
			auto furthestColliderPoint = BoxFurthestPosition(collider, d);
			auto furthestObjectPoint = BoxFurthestPosition(objBox, -d);

			return furthestColliderPoint - furthestObjectPoint;
		};

		auto SameDirection = [](
			const glm::vec3& d,
			const glm::vec3& AO
			)
		{
			return glm::dot(d, AO) > 0.0f;
		};

		auto HandleLine = [SameDirection](std::vector<glm::vec3>& line, glm::vec3 & d) 
		{
			auto& a = line[1];
			auto& b = line[0];
			
			glm::vec3 ab = b - a;
			glm::vec3 ao = origin - a;

			if (SameDirection(ab, ao))
			{
				// Keep both vertices in simplex
				d = glm::cross(glm::cross(ab, ao), ab);
			}
			else
			{
				// Remove B and keep new vertex
				line.erase(line.begin());
				d = ao;
			}

			return false;
		};

		auto HandleTriangle = [&](std::vector<glm::vec3>& tri, glm::vec3& d) 
		{
			auto& a = tri[2];
			auto& b = tri[1];
			auto& c = tri[0];

			auto ab = b - a;
			auto ac = c - a;
			auto ao = origin - a;

			auto abc = glm::cross(ab, ac);

			// Positive side of line AC
			if (SameDirection(glm::cross(abc, ac), ao))
			{
				if (SameDirection(ac, ao))
				{
					// Remove B
					tri.erase(tri.begin() + 1);
					d = glm::cross(glm::cross(ac, ao), ac);
				}
				// Also on positive side of line AB
				else
				{
					// Remove C
					tri.erase(tri.begin());
					// Calculate line with points b and a
					return HandleLine(tri, d);
				}
			}
			// Negative side of AC
			else
			{
				if (SameDirection(glm::cross(ab, abc), ao))
				{
					// Remove C
					tri.erase(tri.begin());
					// Calculate line with points a and b
					return HandleLine(tri, d);
				}
				else
				{
					if (SameDirection(abc, ao))
					{
						// Keep all vertices in simplex
						d = abc;
					}
					else
					{
						// Swap B and C
						std::swap(*tri.begin(), *(tri.begin() + 1));
						d = -abc;
					}
				}
			}

			return false;
		};

		auto HandleTetrahedron = [&](std::vector<glm::vec3>& tet, glm::vec3& D) -> bool
		{
			auto& a = tet[3];
			auto& b = tet[2];
			auto& c = tet[1];
			auto& d = tet[0];
			
			auto ab = b - a;
			auto ac = c - a;
			auto ad = d - a;
			auto ao = origin - a;
			
			auto abc = glm::cross(ab, ac);
			auto acd = glm::cross(ac, ad);
			auto adb = glm::cross(ad, ab);

			if (SameDirection(abc, ao))
			{
				// Remove D
				tet.erase(tet.begin());
				return HandleTriangle(tet, D);
			}
				
			if (SameDirection(acd, ao))
			{
				// Remove B 
				tet.erase(tet.begin() + 2);
				return HandleTriangle(tet, D);
			}
			
			if (SameDirection(adb, ao))
			{
				// Remove C and swap D and B
				tet.erase(tet.begin() + 1);
				std::swap(*tet.begin(), *(tet.begin() + 1));
				return HandleTriangle(tet, D);
			}

			return true;
		};


		glm::vec3 d = { 1.0f, 0.0f, 0.0f };
		std::vector<glm::vec3> simplex;
		// First point is the Minkowski difference vertex
		simplex.push_back(MinkowskiSupport(collider, objBox, d));

		d = origin - simplex[0];
		while (true)
		{
			auto A = MinkowskiSupport(collider, objBox, d);
			// If support doesn't pass origin
			if (glm::dot(A, d) <= 0.0f)
				return false;
			simplex.push_back(A);
			auto len = simplex.size();
			switch (len)
			{
			case 2:
				HandleLine(simplex, d);
				break;
			case 3:
				HandleTriangle(simplex, d);
				break;
			case 4:
				if (HandleTetrahedron(simplex, d))
					return true;
				break;
			default:
				ASSERT(false, "Invalid number of simplex vertices in GJK");
			}
		}
	}


	bool DetectCollisionMid(const Primitives::Box& collider,
							Node* node)
	{
		// Test intersection with grid
		if (!Primitives::BoxBox(collider, node->box))
			return false;

		// Perform GJK if on a leaf
		if (node->leaf)
		{
			int i = 0;
			for (auto& obj : node->objects)
			{
				if (DetectCollisionNarrow(collider, obj))
				{
					obj.colliding = true;
					return true;
				}
				++i;
			}
		}

		// Perform mid-phase collision testing on leaves
		for (auto& child : node->children)
		{
			if (child == nullptr) continue;

			if (DetectCollisionMid(collider, child))
				return true;
		}

		return false;
	}


	bool DetectCollisionBroad(const Primitives::Box& collider,
							  Node* node)
	{
		// Don't intersect with grid, then early out
		if (!Primitives::BoxBox(collider, node->box))
			return false;

		// Otherwise perform mid-phase collision testing
		if (DetectCollisionMid(collider, node))
			return true;

		// No intersection after testing
		return false;
	}

	bool RenderCellsRecursively(vk::CommandBuffer commandBuffer,
						   vk::PipelineLayout pipelineLayout,
						   Node* node)
	{
		bool childExists = false;
		for (Octree::Node * child : node->children)
		{
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
			utils::PushIdentityModel(commandBuffer, pipelineLayout);
			obj.mesh.Bind(commandBuffer);
			
			commandBuffer.pushConstants(
				pipelineLayout,
				vk::ShaderStageFlagBits::eFragment,
				sizeof(glm::mat4), sizeof(utils::UBOColor), &node->color
			);

			//TransformComponent transform = ECS::Get().get<TransformComponent>(obj.entity);
			//transform.PushModel(commandBuffer, pipelineLayout);
			obj.mesh.Draw(commandBuffer);

			Mesh<PosVertex>::Cube.Bind(commandBuffer);
			Primitives::Box& objBox = obj.bb;
			glm::mat4 boxModel = glm::scale(glm::translate(utils::identity, objBox.position), objBox.halfExtent*2.0f);
			commandBuffer.pushConstants(
				pipelineLayout,
				vk::ShaderStageFlagBits::eVertex,
				0, sizeof(glm::mat4), &boxModel
			);
			static glm::vec3 blue = { 0.0f, 1.0f, 0.0f };
			static glm::vec3 red = { 1.0f, 0.0f, 0.0f };

			commandBuffer.pushConstants(
				pipelineLayout,
				vk::ShaderStageFlagBits::eFragment,
				sizeof(glm::mat4), sizeof(utils::UBOColor), (obj.colliding) ? &red : &blue
			);

			obj.colliding = false;
			Mesh<PosVertex>::Cube.Draw(commandBuffer);
		}
	}


	void DestroyRecursively(Node* node)
	{
		for (Octree::Node * child : node->children)
		{
			if (child != nullptr)
				DestroyRecursively(child);
		}
		for (auto& obj : node->objects)
			obj.mesh.Destroy();
		delete node;
	}


	void InsertObject(Node& node, 
					  Mesh<PosVertex>::Data& data,
					  const glm::vec3& cellPosition,
					  float cellHalfExtent)
	{
		int triangleCount = data.indices.size() / 3;

		auto EmplaceObject =
			[this, &node](Mesh<PosVertex>::Data& d)
		{
			node.objects.emplace_back();
			auto& obj = node.objects.back();
			obj.mesh.CreateStatic(d.vertices, d.indices, *m_owner);
			obj.bb = obj.mesh.GetBoundingBox();
			node.leaf = true;
		};

		if (triangleCount < MinimumTriangles)
		{
			EmplaceObject(data);
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
				float minDistance;
				int straddling = Mesh<PosVertex>::IsStraddlingPlane(
					split.data.vertices.data(),
					split.data.vertices.size(),
					planes[i], &minDistance);

				if (minDistance > node.box.halfExtent)
				{
					// Erase this SplitData as it is outside the cell
					splits.erase(splits.begin() + j--);
					--splitSize;
					continue;
				}

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

					// BAIL if we're at the point where splitting increases
					// our triangle count
					if (front.indices.size() / 3 > triangleCount ||
						back.indices.size()  / 3 > triangleCount)
					{
						EmplaceObject(data);
						return;
					}

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
				child->color = glm::vec3(utils::Random(), utils::Random(), utils::Random());
			}

			// Insert into children based on index mask
			InsertObject(*child, split.data, cellPosition + offset, step);
		}
	}

	// TODO: Not hard-code this
	glm::mat4* colliderTransform = nullptr;

	Node* head = nullptr;
	Device* m_owner;
};