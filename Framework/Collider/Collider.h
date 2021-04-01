#pragma once
#include "Primitives/Primitives.hpp"

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
	static Mesh<PosVertex> MeshBox;
	static Mesh<PosVertex> MeshSphere;
	static Mesh<PosVertex> MeshPlane;
	static Mesh<PosVertex> MeshPoint;
	static Mesh<PosVertex> MeshTriangle;
	static Mesh<PosVertex> MeshRay;

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

	Box world;
private:
	Box local;
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

	Sphere world;
private:
	Sphere local;
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

	Plane world;

private:
	Plane local;

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

	Point world;
};

class RayCollider : public Collider
{
public:
	static RayCollider* Create(const Mesh<Vertex>& mesh,
							   const glm::vec3& direction = { 0.0f, 1.0f, 0.0f });

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

	Ray world;
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

	Triangle world;

private:
	Triangle local;
};

