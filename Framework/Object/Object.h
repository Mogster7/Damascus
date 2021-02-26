#pragma once
#include "glm/glm.hpp"
#include "Collider/Collider.h"

class Object
{
public:
	inline static glm::mat4 identity = glm::mat4(1.0f);

	static void PushIdentityModel(vk::CommandBuffer commandBuffer,
								  vk::PipelineLayout pipelineLayout)
	{
		commandBuffer.pushConstants(
			pipelineLayout,
			vk::ShaderStageFlagBits::eVertex,
			0, sizeof(glm::mat4), &identity
		);
	}

	void PushModel(vk::CommandBuffer commandBuffer,
				   vk::PipelineLayout pipelineLayout)
	{
		commandBuffer.pushConstants(
			pipelineLayout,
			vk::ShaderStageFlagBits::eVertex,
			0, sizeof(glm::mat4), &GetModel()
		);
	}

	bool dirty = true;

	glm::mat4 model = glm::mat4(1.0f);

	void Create(Mesh<Vertex>& mesh, 
				Collider::Type colliderType)
	{
		this->mesh = std::move(mesh);
		
		
		switch (colliderType)
		{
		case Collider::Type::Box:
			collider = BoxCollider::Create(mesh);
			break;
		case Collider::Type::Sphere:
			collider = SphereCollider::Create(mesh);
			break;
		default:
			break;
		}
	}


	const glm::vec3& GetPosition() const { return m_position; }
	const glm::vec3& GetScale() const { return m_scale; }
	const glm::vec3& GetRotation() const { return m_rotation; }
	const glm::mat4& GetStoredRotation() const { return m_storedRotMat; }

	void SetPosition(const glm::vec3& translation);
	void SetScale(const glm::vec3& scale);
	void SetRotation(const glm::vec3& rotation);


	void UpdateModel();
	const glm::mat4& GetModel() { if (dirty) UpdateModel(); return model; }

	void Destroy();

	// Mesh data
	Mesh<Vertex> mesh;
	Collider* collider = nullptr;

private:
	// Transform data
	glm::vec3 m_scale = glm::vec3(1.0f);
	glm::vec3 m_rotation = glm::vec3(0.0f);
	glm::vec3 m_position = glm::vec3(0.0f);
	glm::mat4 m_storedRotMat = glm::mat4(1.0f);

};

