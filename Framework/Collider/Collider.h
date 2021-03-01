#pragma once

class Collider
{
public:
	enum class Type { 
		Box, 
		Triangle, 
		Plane, 
		Point, 
		Sphere,
		Ray
	};
	static Mesh<PosVertex> Box;
	static Mesh<PosVertex> Sphere;
	static Mesh<PosVertex> Plane;
	static Mesh<PosVertex> Point;
	static Mesh<PosVertex> Triangle;
	static Mesh<PosVertex> Ray;

	static void InitializeMeshes(Device& device);

	virtual void Update(const Mesh<Vertex>& mesh, 
						const glm::vec3& position,
						const glm::mat4& rotation,
						const glm::vec3& scale) = 0;

	virtual void Draw(vk::CommandBuffer commandBuffer,
					  const glm::vec3& position,
					  const glm::mat4& rotation,
					  const glm::vec3& scale,
					  vk::PipelineLayout layout) const {};

	virtual void TestIntersection(Collider* other) {};

	Type type;
	bool colliding = false;

	struct UBO
	{
		glm::mat4 model;
		int colliding;
	};
	inline static vk::PushConstantRange PushConstant = {
		vk::ShaderStageFlagBits::eVertex,
		0,
		sizeof(UBO)
	};

protected:
	void Push(vk::CommandBuffer cmdBuf, vk::PipelineLayout layout,
			  const glm::mat4& model) const;

};

class BoxCollider : public Collider
{
public:
	static BoxCollider* Create(const Mesh<Vertex>& mesh);


	void UpdateBoundingVolume(const Mesh<Vertex>& mesh);
	void Update(const Mesh<Vertex>& mesh, 
						const glm::vec3& position,
						const glm::mat4& rotation,
						const glm::vec3& scale) override;

	void TestIntersection(Collider* other) override;



	void Draw(vk::CommandBuffer commandBuffer,
			  const glm::vec3& position,
			  const glm::mat4& rotation,
			  const glm::vec3& scale,
			  vk::PipelineLayout layout) const;

	glm::vec3 halfExtent = glm::vec3(0.5f);
	glm::vec3 center;

private:

	glm::vec3 _localHalfExtent = glm::vec3(0.5f);
	glm::vec3 _localCenter;
};


class SphereCollider : public Collider
{
public:
	static SphereCollider* Create(const Mesh<Vertex>& mesh);

	void UpdateBoundingVolume(const Mesh<Vertex>& mesh);

	void Update(const Mesh<Vertex>& mesh, 
						const glm::vec3& position,
						const glm::mat4& rotation,
						const glm::vec3& scale)  override;

	void Draw(vk::CommandBuffer commandBuffer,
			  const glm::vec3& position,
			  const glm::mat4& rotation,
			  const glm::vec3& scale,
			  vk::PipelineLayout layout) const override;

	void TestIntersection(Collider* other) override;

	float radius;
	glm::vec3 center;

private:
	glm::vec3 _localCenter;
	float _localRadius;
};

class PlaneCollider : public Collider
{
public:
	static PlaneCollider* Create(const Mesh<Vertex>& mesh,
								 float thickness = 0.3f);

	void Update(const Mesh<Vertex>& mesh, 
						const glm::vec3& position,
						const glm::mat4& rotation,
						const glm::vec3& scale) override;

	void UpdateBoundingVolume(const Mesh<Vertex>& mesh);

	void Draw(vk::CommandBuffer commandBuffer,
			  const glm::vec3& position,
			  const glm::mat4& rotation,
			  const glm::vec3& scale,
			  vk::PipelineLayout layout) const ;

	void TestIntersection(Collider* other) override;

	glm::vec3 normal;
	glm::vec3 position;
	float D;
	float thickness = 0.1f;
	
private:
	glm::vec3 _localNormal;

};

class PointCollider : public Collider
{
public:
	static PointCollider* Create(const Mesh<Vertex>& mesh);

	void Update(const Mesh<Vertex>& mesh, 
						const glm::vec3& position,
						const glm::mat4& rotation,
						const glm::vec3& scale) override;

	void Draw(vk::CommandBuffer commandBuffer,
			  const glm::vec3& position,
			  const glm::mat4& rotation,
			  const glm::vec3& scale,
			  vk::PipelineLayout layout) const ;

	void TestIntersection(Collider* other) override;

	inline static float visualRadius = 0.3f;

	glm::vec3 position;
};

class RayCollider : public Collider
{
public:
	static RayCollider* Create(const Mesh<Vertex>& mesh,
							   const glm::vec3& direction);

	void Update(const Mesh<Vertex>& mesh, 
						const glm::vec3& position,
						const glm::mat4& rotation,
						const glm::vec3& scale) override;

	void Draw(vk::CommandBuffer commandBuffer,
			  const glm::vec3& position,
			  const glm::mat4& rotation,
			  const glm::vec3& scale,
			  vk::PipelineLayout layout) const ;

	void TestIntersection(Collider* other) override;

	glm::vec3 position;
	glm::vec3 normalizedDirection;
	glm::vec3 inverseNormalizedDirection;
};

class TriangleCollider : public Collider
{
public:
	static TriangleCollider* Create(const Mesh<Vertex>& mesh);

	void Update(const Mesh<Vertex>& mesh, 
						const glm::vec3& position,
						const glm::mat4& rotation,
						const glm::vec3& scale) override;


	void Draw(vk::CommandBuffer commandBuffer,
			  const glm::vec3& position,
			  const glm::mat4& rotation,
			  const glm::vec3& scale,
			  vk::PipelineLayout layout) const ;

	void TestIntersection(Collider* other) override;

	glm::vec3 positions[3];

private:
	glm::vec3 _localPositions[3];

};

