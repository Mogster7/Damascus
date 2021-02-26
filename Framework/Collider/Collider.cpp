#include "Collider.h"

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

	center = (y + z) * 0.5f;
	radius = glm::distance(y, z) * 0.5f;

	for (auto& point : points)
	{
		if (glm::distance(point, center) > radius)
		{
			glm::vec3 d = point - center;
			glm::vec3 dNorm = glm::normalize(d);

			glm::vec3 newPoint = center - radius * dNorm;
			float newRadius = 0.5f * glm::length(point - newPoint);

			center += (newRadius - radius) * dNorm;
			radius = newRadius;
		}
	}

}

void SphereCollider::Update(const Mesh<Vertex>& mesh, 
							const glm::vec3& position, 
							const glm::mat4& rotation, 
							const glm::vec3& scale)
{
	glm::mat4 model = glm::mat4(1.0f);

	glm::mat4 translationMat = glm::translate(model, position);
	glm::mat4 scaleMat = glm::scale(model,  scale);
	
	model = translationMat * scaleMat;

}

void SphereCollider::Draw(vk::CommandBuffer commandBuffer, 
						  const glm::vec3& position,
						  const glm::mat4& rotation,
						  const glm::vec3& scale,
						  vk::PipelineLayout layout) const
{
	glm::mat4 objScaleMat = glm::scale(Object::identity, scale);
	glm::mat4 objTransMat = glm::translate(Object::identity, position);

	auto spherePos = objTransMat * objScaleMat * glm::vec4(center, 1.0f);

	glm::mat4 transMat = glm::translate(Object::identity, (glm::vec3)spherePos);
	float maxAxis = std::max(std::max(scale.x, scale.y), scale.z);
	glm::mat4 scaleMat = glm::scale(Object::identity, glm::vec3(radius) * maxAxis);

	glm::mat4 model = transMat * scaleMat;

	Mesh<PosVertex>::UnitSphere.Bind(commandBuffer);
	Object::PushIdentityModel(commandBuffer, layout);
	Mesh<PosVertex>::UnitSphere.Draw(commandBuffer);
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


	center = glm::vec3(maxX.x + minX.x, maxY.y + minY.y, maxZ.z + minZ.z) * 0.5f;
	halfExtent = glm::vec3(maxX.x - minX.x, maxY.y - minY.y, maxZ.z - minZ.z) * 0.5f;
}

void BoxCollider::Update(const Mesh<Vertex>& mesh, 
						 const glm::vec3& position,
						 const glm::mat4& rotation, 
						 const glm::vec3& scale)
{

	// TODO: Implement on rotation, argh
	//auto halfSize = halfExtent;

	////// Transform the center (as a point)
	//glm::vec3 newHalfSize(0.0f);

	//for (int i = 0; i < 3; ++i)
	//{
	//	newHalfSize[i] = glm::abs(rotation[i][0]) * (halfSize[0])
	//		+ glm::abs(rotation[i][1]) * (halfSize[1])
	//		+ glm::abs(rotation[i][2]) * (halfSize[2]);
	//}

	//halfExtent = newHalfSize;
}

void BoxCollider::Draw(vk::CommandBuffer commandBuffer, 
					   const glm::vec3& position, 
					   const glm::mat4& rotation, 
					   const glm::vec3& scale, 
					   vk::PipelineLayout layout) const
{
	glm::mat4 objScaleMat = glm::scale(Object::identity, scale);
	glm::mat4 objTransMat = glm::translate(Object::identity, position);
	glm::vec3 boxPos = objTransMat * objScaleMat * glm::vec4(center, 1.0f);

	glm::mat4 transMat = glm::translate(Object::identity, boxPos);
	glm::mat4 scaleMat = glm::scale(Object::identity, glm::vec3(halfExtent * scale));

	glm::mat4 model = transMat * scaleMat;

	
	Mesh<PosVertex>::UnitCube.Bind(commandBuffer);
	Push(commandBuffer, layout, model);
	Mesh<PosVertex>::UnitCube.Draw(commandBuffer);
}


void PlaneCollider::Update(const Mesh<Vertex>& mesh, 
						   const glm::vec3& position, 
						   const glm::mat4& rotation, 
						   const glm::vec3& scale)
{
	D = glm::length(position);
	static glm::vec3 originalNormal = normal;
	normal = rotation * glm::vec4(originalNormal, 1.0f);
}

void PlaneCollider::Draw(vk::CommandBuffer commandBuffer, 
						 const glm::vec3& position,
						 vk::PipelineLayout layout) const
{

}
