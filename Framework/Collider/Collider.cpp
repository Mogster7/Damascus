#include "Collider.h"

Mesh<PosVertex> Collider::Box;
Mesh<PosVertex> Collider::Sphere;
Mesh<PosVertex> Collider::Plane;
Mesh<PosVertex> Collider::Ray;
Mesh<PosVertex> Collider::Point;
Mesh<PosVertex> Collider::Triangle;

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

	Box.CreateStatic(cubePosVertices, cubeIndices2, device);
	Sphere.CreateStatic(spherePosVertices, device);

	Plane.CreateStatic(planeVerts, planeIndices, device);

	std::vector<PosVertex> point = { PosVertex({ 0.0f, 0.0f, 0.0f }) };
	Point.CreateStatic(point, device);

	Ray.CreateStatic(rayVerts, device);

	Triangle.CreateStatic(triVerts, device);
}

static bool SphereSphere(SphereCollider& sphere1, SphereCollider& sphere2)
{
	float distance = glm::distance(sphere1.center, sphere2.center);
	return distance < (sphere1.radius + sphere2.radius);
}

static bool BoxSphere(BoxCollider& box, SphereCollider& sphere)
{
	glm::vec3 boxMin = box.center - box.halfExtent;
	glm::vec3 boxMax = box.center + box.halfExtent;
	glm::vec3 closestPoint =
	{
		glm::max(boxMin.x, glm::min(sphere.center.x, boxMax.x)),
		glm::max(boxMin.y, glm::min(sphere.center.y, boxMax.y)),
		glm::max(boxMin.z, glm::min(sphere.center.z, boxMax.z))
	};

	float distance = glm::distance(closestPoint, sphere.center);

	return distance < sphere.radius;
}

static bool BoxBox(BoxCollider& box1, BoxCollider& box2)
{
	glm::vec3 aMin = box1.center - box1.halfExtent;
	glm::vec3 aMax = box1.center + box1.halfExtent;
	glm::vec3 bMin = box2.center - box2.halfExtent;
	glm::vec3 bMax = box2.center + box2.halfExtent;

	return (aMin.x <= bMax.x && aMax.x >= bMin.x) &&
		(aMin.y <= bMax.y && aMax.y >= bMin.y) &&
		(aMin.z <= bMax.z && aMax.z >= bMin.z);
}

static bool PlaneBox(PlaneCollider& plane, BoxCollider& box)
{
	// Compute projection interval radius of b onto the line L = box.center + t * plane.normal
	float r = glm::dot(box.halfExtent, glm::abs(plane.normal));

	// Compute distance of box center from plane
	float s = glm::dot(plane.normal, box.center) - plane.D;

	// Intersection occurs when distance is between [-r,+r]
	return glm::abs(s) <= r;
}

static bool PlaneSphere(PlaneCollider& plane, SphereCollider& sphere)
{
	glm::vec3 v = sphere.center - plane.position;
	float d = glm::dot(plane.normal, v) / glm::sqrt(glm::dot(plane.normal, plane.normal));
	return glm::abs(d) <= sphere.radius;
}

static bool PointBox(PointCollider& point, BoxCollider& box)
{
	glm::vec3 min = box.center - box.halfExtent;
	glm::vec3 max = box.center + box.halfExtent;

	return point.position.x >= min.x && point.position.x <= max.x &&
		point.position.y >= min.y && point.position.y <= max.y &&
		point.position.z >= min.z && point.position.z <= max.z;
}

static bool PointSphere(PointCollider& point, SphereCollider& sphere)
{
	return glm::distance(point.position, sphere.center) <= sphere.radius;
}

static bool PointPlane(PointCollider& point, PlaneCollider& plane)
{
	glm::vec3 v = point.position - plane.position;
	float d = glm::dot(plane.normal, v) / glm::sqrt(glm::dot(plane.normal, plane.normal));
	return glm::abs(d) < plane.thickness;
}

static bool RayBox(RayCollider& ray, BoxCollider& box)
{
	glm::vec3 lb = box.center - box.halfExtent;
	glm::vec3 rt = box.center + box.halfExtent;
	// lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
	float t1 = (lb.x - ray.position.x) * ray.inverseNormalizedDirection.x;
	float t2 = (rt.x - ray.position.x) * ray.inverseNormalizedDirection.x;
	float t3 = (lb.y - ray.position.y) * ray.inverseNormalizedDirection.y;
	float t4 = (rt.y - ray.position.y) * ray.inverseNormalizedDirection.y;
	float t5 = (lb.z - ray.position.z) * ray.inverseNormalizedDirection.z;
	float t6 = (rt.z - ray.position.z) * ray.inverseNormalizedDirection.z;

	float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
	float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

	// if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
	if (tmax < 0)
		return false;

	// if tmin > tmax, ray doesn't intersect AABB
	if (tmin > tmax)
		return false;

	return true;
}

static bool SolveQuadratic(const float& a, const float& b, const float& c, float& x0, float& x1)
{
	float discr = b * b - 4 * a * c;
	if (discr < 0) return false;
	else if (discr == 0) x0 = x1 = -0.5 * b / a;
	else {
		float q = (b > 0) ?
			-0.5 * (b + sqrt(discr)) :
			-0.5 * (b - sqrt(discr));
		x0 = q / a;
		x1 = c / q;
	}
	if (x0 > x1) std::swap(x0, x1);

	return true;
}

static bool RaySphere(RayCollider& ray, SphereCollider& sphere)
{
	float t0, t1;

	glm::vec3 L = ray.position - sphere.center;
	float a = glm::dot(ray.normalizedDirection, ray.normalizedDirection);
	float b = 2 * glm::dot(ray.normalizedDirection, L);
	float c = glm::dot(L, L) - sphere.radius * sphere.radius;

	if (!SolveQuadratic(a, b, c, t0, t1)) return false;

	if (t0 > t1) std::swap(t0, t1);

	if (t0 < 0)
	{
		t0 = t1;
		if (t0 < 0) return false;
	}

	return true;
}

static bool RayPlane(RayCollider& ray, PlaneCollider& plane)
{
	// assuming vectors are all normalized
	float denom = glm::dot(plane.normal, ray.normalizedDirection);
	if (denom > plane.thickness) {
		glm::vec3 p0l0 = plane.position - ray.position;
		float t = glm::dot(p0l0, plane.normal) / denom;
		return (t >= 0);
	}

	return false;
}

static bool TriangleBox(TriangleCollider& tri, BoxCollider& box)
{

}

static bool TrianglePoint(TriangleCollider& tri, PointCollider& point)
{

}

static bool TriangleRay(TriangleCollider& tri, PointCollider& point)
{
	//// compute plane's normal
	//Vec3f v0v1 = v1 - v0;
	//Vec3f v0v2 = v2 - v0;
	//// no need to normalize
	//Vec3f N = v0v1.crossProduct(v0v2); // N 
	//float area2 = N.length();

	//// Step 1: finding P

	//// check if ray and plane are parallel ?
	//float NdotRayDirection = N.dotProduct(dir);
	//if (fabs(NdotRayDirection) < kEpsilon) // almost 0 
	//	return false; // they are parallel so they don't intersect ! 

	//// compute d parameter using equation 2
	//float d = N.dotProduct(v0);

	//// compute t (equation 3)
	//t = (N.dotProduct(orig) + d) / NdotRayDirection;
	//// check if the triangle is in behind the ray
	//if (t < 0) return false; // the triangle is behind 

	//// compute the intersection point using equation 1
	//Vec3f P = orig + t * dir;

	//// Step 2: inside-outside test
	//Vec3f C; // vector perpendicular to triangle's plane 

	//// edge 0
	//Vec3f edge0 = v1 - v0;
	//Vec3f vp0 = P - v0;
	//C = edge0.crossProduct(vp0);
	//if (N.dotProduct(C) < 0) return false; // P is on the right side 

	//// edge 1
	//Vec3f edge1 = v2 - v1;
	//Vec3f vp1 = P - v1;
	//C = edge1.crossProduct(vp1);
	//if (N.dotProduct(C) < 0)  return false; // P is on the right side 

	//// edge 2
	//Vec3f edge2 = v0 - v2;
	//Vec3f vp2 = P - v2;
	//C = edge2.crossProduct(vp2);
	//if (N.dotProduct(C) < 0) return false; // P is on the right side; 

	//return true; // this ray hits the triangle 
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

	_localCenter = (y + z) * 0.5f;
	_localRadius = glm::distance(y, z) * 0.5f;

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
	glm::mat4 objScaleMat = glm::scale(Object::identity, scale);
	glm::mat4 objTransMat = glm::translate(Object::identity, position);
	center = objTransMat * objScaleMat * glm::vec4(_localCenter, 1.0f);

	radius = _localRadius * std::max(std::max(scale.x, scale.y), scale.z);
}

void SphereCollider::TestIntersection(Collider* other)
{
	bool collided = false;
	switch (other->type)
	{
	case Type::Sphere:
		collided = SphereSphere(*this, *static_cast<SphereCollider*>(other));
		break;
	case Type::Box:
		collided = BoxSphere(*static_cast<BoxCollider*>(other), *this);
		break;
	case Type::Point:
		collided = PointSphere(*static_cast<PointCollider*>(other), *this);
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
	glm::mat4 transMat = glm::translate(Object::identity, center);
	glm::mat4 scaleMat = glm::scale(Object::identity, glm::vec3(radius) * 2.0f);
	glm::mat4 model = transMat * scaleMat;

	Sphere.Bind(commandBuffer);
	Push(commandBuffer, layout, model);
	Sphere.Draw(commandBuffer);
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


	_localCenter = glm::vec3(maxX.x + minX.x, maxY.y + minY.y, maxZ.z + minZ.z) * 0.5f;
	_localHalfExtent = glm::vec3(maxX.x - minX.x, maxY.y - minY.y, maxZ.z - minZ.z) * 0.5f;
}

void BoxCollider::Update(const Mesh<Vertex>& mesh, 
						 const glm::vec3& position,
						 const glm::mat4& rotation, 
						 const glm::vec3& scale)
{
	glm::mat4 objScaleMat = glm::scale(Object::identity, scale);
	glm::mat4 objTransMat = glm::translate(Object::identity, position);
	center = objTransMat * objScaleMat * glm::vec4(_localCenter, 1.0f);

	halfExtent = _localHalfExtent * scale;
	
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

void BoxCollider::TestIntersection(Collider* other)
{
	bool collided = false;
	switch (other->type)
	{
	case Type::Sphere:
		collided = BoxSphere(*this, *static_cast<SphereCollider*>(other));
		break;
	case Type::Box:
		collided = BoxBox(*this, *static_cast<BoxCollider*>(other));
		break;
	case Type::Plane:
		collided = PlaneBox(*static_cast<PlaneCollider*>(other), *this);
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
	glm::mat4 transMat = glm::translate(Object::identity, center);
	glm::mat4 scaleMat = glm::scale(Object::identity, glm::vec3(halfExtent) * 2.0f);

	glm::mat4 model = transMat * scaleMat;
	
	Box.Bind(commandBuffer);
	Push(commandBuffer, layout, model);
	Box.Draw(commandBuffer);
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
	_localNormal = { 0.0f, 1.0f, 0.0f };
}

void PlaneCollider::Update(const Mesh<Vertex>& mesh, 
						   const glm::vec3& position, 
						   const glm::mat4& rotation, 
						   const glm::vec3& scale)
{
	this->position = position;
	normal = rotation * glm::vec4(_localNormal, 1.0f);
	D = glm::dot(position, normal);
}

void PlaneCollider::Draw(vk::CommandBuffer commandBuffer, 
						 const glm::vec3& position,
						 const glm::mat4& rotation,
						 const glm::vec3& scale,
						 vk::PipelineLayout layout) const
{
	glm::mat4 transMat = glm::translate(Object::identity, position);
	glm::mat4 scaleMat = glm::scale(Object::identity, scale);
	
	Plane.Bind(commandBuffer);
	Push(commandBuffer, layout, transMat * scaleMat);
	Plane.Draw(commandBuffer);
}

void PlaneCollider::TestIntersection(Collider* other)
{
	bool collided = false;
	switch (other->type)
	{
	case Type::Box:
		collided = PlaneBox(*this, *static_cast<BoxCollider*>(other));
		break;
	case Type::Sphere:
		collided = PlaneSphere(*this, *static_cast<SphereCollider*>(other));
		break;
	case Type::Point:
		collided = PointPlane(*static_cast<PointCollider*>(other), *this);
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
	this->position = position;
}

void PointCollider::Draw(vk::CommandBuffer commandBuffer, 
						 const glm::vec3& position, 
						 const glm::mat4& rotation, 
						 const glm::vec3& scale, 
						 vk::PipelineLayout layout) const
{
	glm::mat4 transMat = glm::translate(Object::identity, position);
	glm::mat4 scaleMat = glm::scale(Object::identity, glm::vec3(visualRadius * 2.0f));

	Sphere.Bind(commandBuffer);
	Push(commandBuffer, layout, transMat * scaleMat);
	Sphere.Draw(commandBuffer);
}

void PointCollider::TestIntersection(Collider* other)
{
	bool collided = false;
	switch (other->type)
	{
	case Type::Box:
		collided = PointBox(*this, *static_cast<BoxCollider*>(other));
		break;
	case Type::Sphere:
		collided = PointSphere(*this, *static_cast<SphereCollider*>(other));
		break;
	case Type::Plane:
		collided = PointPlane(*this, *static_cast<PlaneCollider*>(other));
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
	collider->normalizedDirection = glm::normalize(direction);
	collider->inverseNormalizedDirection = 1.0f / collider->normalizedDirection;
	return collider;

}

void RayCollider::Update(const Mesh<Vertex>& mesh, 
						 const glm::vec3& position, 
						 const glm::mat4& rotation, 
						 const glm::vec3& scale)
{
	this->position = position;
	//static glm::vec3 initDirection = normalizedDirection;
	//normalizedDirection = initDirection * scale;
	//static glm::vec3 initInverseDirection = inverseNormalizedDirection;
	//inverseNormalizedDirection = 1.0f / normalizedDirection;
}

void RayCollider::Draw(vk::CommandBuffer commandBuffer, const glm::vec3& position, const glm::mat4& rotation, const glm::vec3& scale, vk::PipelineLayout layout) const
{
	//glm::mat4 rotMat = glm::mat4_cast(glm::quatLookAt(normalizedDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
	glm::mat4 scaleMat = glm::scale(Object::identity, scale);
	glm::mat4 transMat = glm::translate(Object::identity, position);

	Ray.Bind(commandBuffer);
	Push(commandBuffer, layout, transMat * scaleMat);
	Ray.Draw(commandBuffer);
}

void RayCollider::TestIntersection(Collider* other)
{
	bool collided = false;
	switch (other->type)
	{
	case Type::Box:
		collided = RayBox(*this, *static_cast<BoxCollider*>(other));
		break;
	case Type::Sphere:
		collided = RaySphere(*this, *static_cast<SphereCollider*>(other));
		break;
	case Type::Plane:
		collided = RayPlane(*this, *static_cast<PlaneCollider*>(other));
		break;
	}
	colliding |= collided;
	other->colliding |= collided;
}

TriangleCollider* TriangleCollider::Create(const Mesh<Vertex>& mesh)
{
	auto* collider = new TriangleCollider();

	return collider;
}

void TriangleCollider::Update(const Mesh<Vertex>& mesh, const glm::vec3& position, const glm::mat4& rotation, const glm::vec3& scale)
{

}

void TriangleCollider::Draw(vk::CommandBuffer commandBuffer, const glm::vec3& position, const glm::mat4& rotation, const glm::vec3& scale, vk::PipelineLayout layout) const
{

}

void TriangleCollider::TestIntersection(Collider* other)
{

}
