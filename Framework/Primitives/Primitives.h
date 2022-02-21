#pragma once

namespace dm::primitives
{
	struct Box
	{
		glm::vec3 position;
		glm::vec3 halfExtent;
	};

	struct BoxUniform
	{
		glm::vec3 position;
		float halfExtent;
	};

	struct Sphere
	{
		float radius;
		glm::vec3 position;
	};

	struct Plane
	{
		glm::vec3 normal;
		glm::vec3 position;
		float D;
		inline static float thickness = glm::epsilon<float>();
	};

	struct Point
	{
		glm::vec3 position;
	};

	struct Ray
	{
		glm::vec3 position;
		glm::vec3 direction;
	};

	struct Triangle
	{
		glm::vec3 positions[3];
	};

	enum PointPlaneStatus
	{
		Back = 0,
		Front,
		Coplanar
	};

	static Plane GeneratePlane(glm::vec3 p[3])
	{
		glm::vec3 v1 = glm::normalize(p[1] - p[0]);
		glm::vec3 v2 = glm::normalize(p[2] - p[0]);

		return { glm::normalize(glm::cross(v1, v2)), (p[0] + p[1] + p[2]) / 3.0f };
	}

	static Plane GeneratePlaneBetweenTwoPoints(glm::vec3& a, glm::vec3& b)
	{
		Plane plane{};
		plane.position = (a + b) * 0.5f;
		plane.normal = glm::normalize(b - a);

		return plane;
	}



	static int ClassifyPointToPlane(const glm::vec3& point, const Plane& plane)
	{
		float distance = glm::dot(plane.normal, point - plane.position);
		if (distance > Plane::thickness)
		{
			return PointPlaneStatus::Front;
		}
		if (distance < -Plane::thickness)
		{
			return PointPlaneStatus::Back;
		}

		return PointPlaneStatus::Coplanar;
	}

	static int ClassifyPointToPlane(const Point& point, const Plane& plane)
	{
		return ClassifyPointToPlane(point.position, plane);
	}

	static glm::vec3 GetRayPlaneIntersection(const Ray& ray, const Plane& plane)
	{
		glm::vec3 diff = ray.position - plane.position;
		float d1 = glm::dot(diff, plane.normal);
		double d2 = glm::dot(ray.direction, plane.normal);
		float t = (glm::dot(plane.normal, plane.position) - glm::dot(plane.normal, ray.position)) /
			d2;
		return ray.position + t * ray.direction;
	}

	static bool SphereSphere(Sphere& sphere1, Sphere& sphere2)
	{
		float distance = glm::distance(sphere1.position, sphere2.position);
		return distance < (sphere1.radius + sphere2.radius);
	}

	static bool BoxSphere(Box& box, Sphere& sphere)
	{
		glm::vec3 boxMin = box.position - box.halfExtent;
		glm::vec3 boxMax = box.position + box.halfExtent;
		glm::vec3 closestPoint =
		{
			glm::max(boxMin.x, glm::min(sphere.position.x, boxMax.x)),
			glm::max(boxMin.y, glm::min(sphere.position.y, boxMax.y)),
			glm::max(boxMin.z, glm::min(sphere.position.z, boxMax.z))
		};

		float distance = glm::distance(closestPoint, sphere.position);

		return distance < sphere.radius;
	}

	static bool SphereBox(Sphere& sphere, Box& box)
	{
		return BoxSphere(box, sphere);
	}

	template <class Box1, class Box2>
	static bool BoxBox(Box1& box1, Box2& box2)
	{
		glm::vec3 aMin = box1.position - box1.halfExtent;
		glm::vec3 aMax = box1.position + box1.halfExtent;
		glm::vec3 bMin = box2.position - box2.halfExtent;
		glm::vec3 bMax = box2.position + box2.halfExtent;

		return (aMin.x <= bMax.x && aMax.x >= bMin.x) &&
			(aMin.y <= bMax.y && aMax.y >= bMin.y) &&
			(aMin.z <= bMax.z && aMax.z >= bMin.z);
	}

	static glm::vec3 BoxFurthestPosition(const Box& box, const glm::vec3& direction)
	{
		auto& h = box.halfExtent;
		glm::vec3 furthest;
		float maxDot = -FLT_MAX;
		glm::vec3 offsets[8] = {
			{ -h.x, -h.y, -h.z },
			{ h.x, -h.y, -h.z },
			{ -h.x, h.y, -h.z },
			{ h.x, h.y, -h.z },

			{ -h.x, -h.y, h.z },
			{ h.x, -h.y, h.z },
			{ -h.x, h.y, h.z },
			{ h.x, h.y, h.z }
		};

		for (int i = 0; i < 8; ++i)
		{
			auto pos = box.position + offsets[i];

			float dotProduct = glm::dot(glm::normalize(pos - box.position), direction);
			if (dotProduct > maxDot)
			{
				maxDot = dotProduct;
				furthest = pos;
			}
		}

		return furthest;
	}

	static bool PlaneBox(Plane& plane, Box& box)
	{
		// Compute projection interval radius of b onto the line L = box.position + t * plane.normal
		float r = glm::dot(box.halfExtent, glm::abs(plane.normal));

		// Compute distance of box.position from plane
		float s = glm::dot(plane.normal, box.position) - plane.D;

		// Intersection occurs when distance is between [-r,+r]
		return glm::abs(s) <= r;
	}

	static bool BoxPlane(Box& box, Plane& plane)
	{
		return PlaneBox(plane, box);
	}



	static bool PlaneSphere(Plane& plane, Sphere& sphere)
	{
		glm::vec3 v = sphere.position - plane.position;
		float d = glm::dot(plane.normal, v) / glm::sqrt(glm::dot(plane.normal, plane.normal));
		return glm::abs(d) <= sphere.radius;
	}

	static bool SpherePlane(Sphere& sphere, Plane& plane)
	{
		return PlaneSphere(plane, sphere);
	}

	static bool PointBox(Point& point, Box& box)
	{
		glm::vec3 min = box.position - box.halfExtent;
		glm::vec3 max = box.position + box.halfExtent;

		return point.position.x >= min.x && point.position.x <= max.x &&
			point.position.y >= min.y && point.position.y <= max.y &&
			point.position.z >= min.z && point.position.z <= max.z;
	}

	static bool BoxPoint(Box& box, Point& point)
	{
		return PointBox(point, box);
	}


	static bool PointSphere(Point& point, Sphere& sphere)
	{
		return glm::distance(point.position, sphere.position) <= sphere.radius;
	}

	static bool SpherePoint(Sphere& sphere, Point& point)
	{
		return PointSphere(point, sphere);
	}

	static bool PointPlane(Point& point, Plane& plane)
	{
		glm::vec3 v = point.position - plane.position;
		float d = glm::dot(plane.normal, v) / glm::sqrt(glm::dot(plane.normal, plane.normal));
		return glm::abs(d) < plane.thickness;
	}

	static bool PlanePoint(Plane& plane, Point& point)
	{
		return PointPlane(point, plane);
	}


	static bool RayBox(Ray& ray, Box& box)
	{
		glm::vec3 lb = box.position - box.halfExtent;
		glm::vec3 rt = box.position + box.halfExtent;

		glm::vec3 invDir = 1.0f / ray.direction;
		// lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
		float t1 = (lb.x - ray.position.x) * invDir.x;
		float t2 = (rt.x - ray.position.x) * invDir.x;
		float t3 = (lb.y - ray.position.y) * invDir.y;
		float t4 = (rt.y - ray.position.y) * invDir.y;
		float t5 = (lb.z - ray.position.z) * invDir.z;
		float t6 = (rt.z - ray.position.z) * invDir.z;

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

	static bool RaySphere(Ray& ray, Sphere& sphere)
	{
		float t0, t1;

		glm::vec3 L = ray.position - sphere.position;
		float a = glm::dot(ray.direction, ray.direction);
		float b = 2 * glm::dot(ray.direction, L);
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

	static bool RayPlane(Ray& ray, Plane& plane)
	{
		// assuming vectors are all normalized
		float denom = glm::dot(plane.normal, ray.direction);
		return (denom > plane.thickness || denom < -plane.thickness);
		/*	{
				glm::vec3 p0l0 = plane.position - ray.position;
				float t = glm::dot(p0l0, plane.normal) / denom;
				return (t >= 0);
			*/
			//	}
	}
}
