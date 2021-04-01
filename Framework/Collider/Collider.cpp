#include "Collider.h"

Mesh<PosVertex> Collider::MeshBox;
Mesh<PosVertex> Collider::MeshSphere;
Mesh<PosVertex> Collider::MeshPlane;
Mesh<PosVertex> Collider::MeshRay;
Mesh<PosVertex> Collider::MeshPoint;
Mesh<PosVertex> Collider::MeshTriangle;

const std::vector<PosVertex> planeVerts = {
	 PosVertex({ -1.0, 0.0, -1.0 }),	// 0
	 PosVertex({ -1.0, 0.0, 1.0 }),	    // 1
	 PosVertex({ 1.0, 0.0, 1.0 }),    // 2
	 PosVertex({ 1.0, 0.0, -1.0 })   // 3
};

const std::vector<uint32_t> planeIndices = {
	0, 1, 2,
	2, 3, 0
};

const std::vector<PosVertex> rayVerts = {
	 PosVertex({ 0.0, 0.0, 0.0 }),	// 0
	 PosVertex({ 0.0, 0.0, 1.0 })   // 1
};

const std::vector<PosVertex> triVerts = {
	 PosVertex({ 0.0, 1.0, 0.0 }),	// 0
	 PosVertex({ 1.0, 0.0, 0.0 }) ,  // 1
	 PosVertex({ -1.0, 0.0, 0.0 })   // 2
};



void Collider::InitializeMeshes(Device& device)
{
	auto cubePositions = Mesh<Vertex>::UnitCube.GetVertexBufferData<glm::vec3>(0);
	auto cubeIndices2 = Mesh<Vertex>::UnitCube.GetIndexBufferData();

	std::vector<PosVertex> cubePosVertices(cubePositions.begin(), cubePositions.end());

	auto spherePositions2 = Mesh<Vertex>::UnitSphere.GetVertexBufferData<glm::vec3>(0);
	std::vector<PosVertex> spherePosVertices(spherePositions2.begin(), spherePositions2.end());

	std::vector<PosVertex> point = { PosVertex({ 0.0f, 0.0f, 0.0f }) };

	MeshBox.CreateStatic(cubePosVertices, cubeIndices2, device);
	MeshSphere.CreateStatic(spherePosVertices, device);
	MeshPlane.CreateStatic(planeVerts, planeIndices, device);
	MeshPoint.CreateStatic(point, device);
	MeshRay.CreateStatic(rayVerts, device);
	MeshTriangle.CreateStatic(triVerts, device);
}



void Collider::Push(vk::CommandBuffer cmdBuf, vk::PipelineLayout layout,
					const glm::mat4& model) const
{
	UBO ubo = { model, colliding };
	cmdBuf.pushConstants(
		layout,
		vk::ShaderStageFlagBits::eVertex,
		0, sizeof(UBO), &ubo
	);
}


SphereCollider* SphereCollider::Create(const Mesh<Vertex>& mesh)
{
	auto* collider = new SphereCollider();
	collider->type = Collider::Type::Sphere;
	collider->UpdateBoundingVolume(mesh);
	return collider;
}

void SphereCollider::UpdateBoundingVolume(const Mesh<Vertex>& mesh)
{
	std::vector<glm::vec3> points = mesh.GetVertexBufferData<glm::vec3>(0);
	for (auto& point : points)
		point = glm::vec4(point, 0.0f);


	glm::vec3 x = points[0];
	glm::vec3 y = *std::max_element(points.begin(), points.end(), [&x](const auto& point1, const auto& point2) {
		return glm::distance(x, point1) < glm::distance(x, point2);
	});
	glm::vec3 z = *std::max_element(points.begin(), points.end(), [&y](const auto& point1, const auto& point2) {
		return glm::distance(y, point1) < glm::distance(y, point2);
	});

	local.position = (y + z) * 0.5f;
	local.radius = glm::distance(y, z) * 0.5f;

	for (auto& point : points)
	{
		if (glm::distance(point, world.position) > world.radius)
		{
			glm::vec3 d = point - world.position;
			glm::vec3 dNorm = glm::normalize(d);

			glm::vec3 newPoint = world.position - world.radius * dNorm;
			float newRadius = 0.5f * glm::length(point - newPoint);

			world.position += (newRadius - world.radius) * dNorm;
			world.radius = newRadius;
		}
	}
}

void SphereCollider::Update(const Mesh<Vertex>& mesh, 
						  const glm::vec3& position,
						  const glm::mat4& rotation,
						  const glm::vec3& scale) 
{
	glm::mat4 objScaleMat = glm::scale(utils::identity, scale);
	glm::mat4 objTransMat = glm::translate(utils::identity, position);
	world.position = objTransMat * objScaleMat * glm::vec4(local.position, 1.0f);

	world.radius = local.radius * std::max(std::max(scale.x, scale.y), scale.z);
}

void SphereCollider::TestIntersection(Collider* other)
{
	bool collided = false;
	switch (other->type)
	{
	case Type::Sphere:
		collided = SphereSphere(world, static_cast<SphereCollider*>(other)->world);
		break;
	case Type::Box:
		collided = BoxSphere(static_cast<BoxCollider*>(other)->world, world);
		break;
	case Type::Point:
		collided = PointSphere(static_cast<PointCollider*>(other)->world, world);
		break;
	}
	colliding |= collided;
	other->colliding |= collided;
}


void SphereCollider::Draw(vk::CommandBuffer commandBuffer, 
						  const glm::vec3& position,
						  const glm::mat4& rotation,
						  const glm::vec3& scale,
						  vk::PipelineLayout layout) const
{
	glm::mat4 transMat = glm::translate(utils::identity, world.position);
	glm::mat4 scaleMat = glm::scale(utils::identity, glm::vec3(world.radius) * 2.0f);
	glm::mat4 model = transMat * scaleMat;

	MeshSphere.Bind(commandBuffer);
	Push(commandBuffer, layout, model);
	MeshSphere.Draw(commandBuffer);
}

BoxCollider* BoxCollider::Create(const Mesh<Vertex>& mesh)
{
	auto* collider = new BoxCollider();
	collider->type = Collider::Type::Box;
	collider->UpdateBoundingVolume(mesh);
	return collider;
}

void BoxCollider::UpdateBoundingVolume(const Mesh<Vertex>& mesh)
{
	std::vector<glm::vec3> points = mesh.GetVertexBufferData<glm::vec3>(0);

	glm::vec3 minX, minY, minZ;
	glm::vec3 maxX, maxY, maxZ;

	auto GetExtremePoints = [&points](auto& min, auto& max, const auto& dir)
	{
		float minProj = FLT_MAX; float maxProj = -FLT_MAX;
		int indexMin = 0, indexMax = 0;

		for (int i = 0; i < points.size(); ++i)
		{
			float proj = glm::dot(points[i], dir);

			if (proj < minProj)
			{
				minProj = proj;
				indexMin = i;
			}
			if (proj > maxProj)
			{
				maxProj = proj;
				indexMax = i;
			}
		}
		min = points[indexMin];
		max = points[indexMax];
	};

	GetExtremePoints(minX, maxX, glm::vec3(1.0f, 0.0f, 0.0f));
	GetExtremePoints(minY, maxY, glm::vec3(0.0f, 1.0f, 0.0f));
	GetExtremePoints(minZ, maxZ, glm::vec3(0.0f, 0.0f, 1.0f));


	local.position = glm::vec3(maxX.x + minX.x, maxY.y + minY.y, maxZ.z + minZ.z) * 0.5f;
	local.halfExtent = glm::vec3(maxX.x - minX.x, maxY.y - minY.y, maxZ.z - minZ.z) * 0.5f;
}

void BoxCollider::Update(const Mesh<Vertex>& mesh, 
						 const glm::vec3& position,
						 const glm::mat4& rotation, 
						 const glm::vec3& scale)
{
	glm::mat4 objScaleMat = glm::scale(utils::identity, scale);
	glm::mat4 objTransMat = glm::translate(utils::identity, position);
	world.position = objTransMat * objScaleMat * glm::vec4(local.position, 1.0f);

	world.halfExtent = local.halfExtent * scale;
	
	// TODO: Implement on rotation, argh
	//auto halfSize = world.halfExtent;

	////// Transform the world.position (as a point)
	//glm::vec3 newHalfSize(0.0f);

	//for (int i = 0; i < 3; ++i)
	//{
	//	newHalfSize[i] = glm::abs(rotation[i][0]) * (halfSize[0])
	//		+ glm::abs(rotation[i][1]) * (halfSize[1])
	//		+ glm::abs(rotation[i][2]) * (halfSize[2]);
	//}

	//world.halfExtent = newHalfSize;
}

void BoxCollider::TestIntersection(Collider* other)
{
	bool collided = false;
	switch (other->type)
	{
	case Type::Sphere:
		collided = BoxSphere(world, static_cast<SphereCollider*>(other)->world);
		break;
	case Type::Box:
		collided = BoxBox(world, static_cast<BoxCollider*>(other)->world);
		break;
	case Type::Plane:
		collided = PlaneBox(static_cast<PlaneCollider*>(other)->world, world);
		break;
	}
	colliding |= collided;
	other->colliding |= collided;
}

void BoxCollider::Draw(vk::CommandBuffer commandBuffer, 
					   const glm::vec3& position, 
					   const glm::mat4& rotation, 
					   const glm::vec3& scale, 
					   vk::PipelineLayout layout) const
{
	glm::mat4 transMat = glm::translate(utils::identity, world.position);
	glm::mat4 scaleMat = glm::scale(utils::identity, glm::vec3(world.halfExtent) * 2.0f);

	glm::mat4 model = transMat * scaleMat;
	
	MeshBox.Bind(commandBuffer);
	Push(commandBuffer, layout, model);
	MeshBox.Draw(commandBuffer);
}

PlaneCollider* PlaneCollider::Create(const Mesh<Vertex>& mesh,
									 float thickness)
{
	auto* collider = new PlaneCollider();
	collider->type = Collider::Type::Plane;
	collider->UpdateBoundingVolume(mesh);
	return collider;
}

void PlaneCollider::UpdateBoundingVolume(const Mesh<Vertex>& mesh)
{
	local.normal = { 0.0f, 1.0f, 0.0f };
}

void PlaneCollider::Update(const Mesh<Vertex>& mesh, 
						   const glm::vec3& position, 
						   const glm::mat4& rotation, 
						   const glm::vec3& scale)
{
	world.position = position;
	world.normal = rotation * glm::vec4(local.normal, 1.0f);
	world.D = glm::dot(position, world.normal);
}

void PlaneCollider::Draw(vk::CommandBuffer commandBuffer, 
						 const glm::vec3& position,
						 const glm::mat4& rotation,
						 const glm::vec3& scale,
						 vk::PipelineLayout layout) const
{
	glm::mat4 transMat = glm::translate(utils::identity, position);
	glm::mat4 scaleMat = glm::scale(utils::identity, scale);
	
	MeshPlane.Bind(commandBuffer);
	Push(commandBuffer, layout, transMat * rotation * scaleMat);
	MeshPlane.Draw(commandBuffer);
}

void PlaneCollider::TestIntersection(Collider* other)
{
	bool collided = false;
	switch (other->type)
	{
	case Type::Box:
		collided = PlaneBox(world, static_cast<BoxCollider*>(other)->world);
		break;
	case Type::Sphere:
		collided = PlaneSphere(world, static_cast<SphereCollider*>(other)->world);
		break;
	case Type::Point:
		collided = PointPlane(static_cast<PointCollider*>(other)->world, world);
		break;
	}
	colliding |= collided;
	other->colliding |= collided;
}

PointCollider* PointCollider::Create(const Mesh<Vertex>& mesh)
{
	auto* collider = new PointCollider();
	collider->type = Collider::Type::Point;
	return collider;
}

void PointCollider::Update(const Mesh<Vertex>& mesh, 
						   const glm::vec3& position, 
						   const glm::mat4& rotation, 
						   const glm::vec3& scale)
{
	world.position = position;
}

void PointCollider::Draw(vk::CommandBuffer commandBuffer, 
						 const glm::vec3& position, 
						 const glm::mat4& rotation, 
						 const glm::vec3& scale, 
						 vk::PipelineLayout layout) const
{
	glm::mat4 transMat = glm::translate(utils::identity, position);
	glm::mat4 scaleMat = glm::scale(utils::identity, glm::vec3(visualRadius * 2.0f));

	MeshSphere.Bind(commandBuffer);
	Push(commandBuffer, layout, transMat * scaleMat);
	MeshSphere.Draw(commandBuffer);
}

void PointCollider::TestIntersection(Collider* other)
{
	bool collided = false;
	switch (other->type)
	{
	case Type::Box:
		collided = PointBox(world, static_cast<BoxCollider*>(other)->world);
		break;
	case Type::Sphere:
		collided = PointSphere(world, static_cast<SphereCollider*>(other)->world);
		break;
	case Type::Plane:
		collided = PointPlane(world, static_cast<PlaneCollider*>(other)->world);
		break;
	}
	colliding |= collided;
	other->colliding |= collided;

}

RayCollider* RayCollider::Create(const Mesh<Vertex>& mesh, 
								 const glm::vec3& direction)
{
	auto* collider = new RayCollider();
	collider->type = Collider::Type::Ray;
	collider->world.direction = glm::normalize(direction);
	return collider;
}

void RayCollider::Update(const Mesh<Vertex>& mesh, 
						 const glm::vec3& position, 
						 const glm::mat4& rotation, 
						 const glm::vec3& scale)
{
	world.position = position;
	//static glm::vec3 initDirection = world.direction;
	//world.direction = initDirection * scale;
	//static glm::vec3 initInverseDirection = inverseNormalizedDirection;
	//inverseNormalizedDirection = 1.0f / world.direction;
}

void RayCollider::Draw(vk::CommandBuffer commandBuffer, 
					   const glm::vec3& position, 
					   const glm::mat4& rotation, 
					   const glm::vec3& scale, 
					   vk::PipelineLayout layout) const
{
	glm::mat4 scaleMat = glm::scale(utils::identity, scale);
	glm::mat4 transMat = glm::translate(utils::identity, position);

	MeshRay.Bind(commandBuffer);
	Push(commandBuffer, layout, transMat * rotation * scaleMat);
	MeshRay.Draw(commandBuffer);
}

void RayCollider::TestIntersection(Collider* other)
{
	bool collided = false;
	switch (other->type)
	{
	case Type::Box:
		collided = RayBox(world, static_cast<BoxCollider*>(other)->world);
		break;
	case Type::Sphere:
		collided = RaySphere(world, static_cast<SphereCollider*>(other)->world);
		break;
	case Type::Plane:
		collided = RayPlane(world, static_cast<PlaneCollider*>(other)->world);
		break;
	}
	colliding |= collided;
	other->colliding |= collided;
}

TriangleCollider* TriangleCollider::Create(const Mesh<Vertex>& mesh)
{
	return new TriangleCollider();
}

void TriangleCollider::Update(const Mesh<Vertex>& mesh, 
							  const glm::vec3& position, 
							  const glm::mat4& rotation, 
							  const glm::vec3& scale)
{
	
}

void TriangleCollider::Draw(vk::CommandBuffer commandBuffer, 
							const glm::vec3& position, 
							const glm::mat4& rotation, 
							const glm::vec3& scale, 
							vk::PipelineLayout layout) const
{

}

void TriangleCollider::TestIntersection(Collider* other)
{

}
