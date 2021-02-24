#pragma once

class Collider
{
public:
	enum class Type { 
		Box, 
		Triangle, 
		Plane, 
		Point, 
		Sphere 
	};

	virtual void Update(const Mesh<Vertex>& mesh, 
						const glm::vec3& position,
						const glm::mat4& rotation,
						const glm::vec3& scale) = 0;

	virtual void Draw(vk::CommandBuffer commandBuffer,
					  const glm::vec3& position,
					  const glm::mat4& rotation,
					  const glm::vec3& scale,
					  vk::PipelineLayout layout) const {};

	Type type;
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

	void Draw(vk::CommandBuffer commandBuffer,
			  const glm::vec3& position,
			  const glm::mat4& rotation,
			  const glm::vec3& scale,
			  vk::PipelineLayout layout) const;

	glm::vec3 halfExtent = glm::vec3(0.5f);
	glm::vec3 center;
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

	float radius;
	glm::vec3 center;
};

class PlaneCollider : public Collider
{
public:

	void Update(const Mesh<Vertex>& mesh, 
						const glm::vec3& position,
						const glm::mat4& rotation,
						const glm::vec3& scale) override;

	void Draw(vk::CommandBuffer commandBuffer,
			  const glm::vec3& position,
			  vk::PipelineLayout layout) const ;

	glm::vec3 normal;
	float D;
};

