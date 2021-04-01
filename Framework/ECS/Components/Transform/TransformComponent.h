#pragma once

class TransformComponent
{
public:
	TransformComponent(glm::vec3 position = { 0.0f, 0.0f, 0.0f },
					   glm::vec3 scale = { 1.0f, 1.0f, 1.0f },
					   glm::vec3 rotation = { 0.0f, 0.0f, 0.0f })
		:
		m_position(position),
		m_scale(scale),
		m_rotation(rotation)
	{
		
	}

	glm::mat4 model = glm::mat4(1.0f);
	bool dirty = false;

	void PushModel(vk::CommandBuffer commandBuffer,
				   vk::PipelineLayout pipelineLayout) const
	{
		commandBuffer.pushConstants(
			pipelineLayout,
			vk::ShaderStageFlagBits::eVertex,
			0, sizeof(glm::mat4), &model
		);
	}
	void UpdateModel();

	inline const glm::vec3& GetPosition() const { return m_position; }
	inline const glm::vec3& GetScale() const { return m_scale; }
	inline const glm::vec3& GetRotation() const { return m_rotation; }
	inline const glm::mat4& GetStoredRotation() const { return m_storedRotMat; }
	inline const glm::mat4& GetModel() { if (dirty) UpdateModel(); return model; }
	void SetPosition(const glm::vec3& translation);
	void SetScale(const glm::vec3& scale);
	void SetRotation(const glm::vec3& rotation);


private:
	glm::vec3 m_position = { 0.0f, 0.0f, 0.0f };
	glm::vec3 m_scale = { 1.0f, 1.0f, 1.0f };
	glm::vec3 m_rotation = { 0.0f, 0.0f, 0.0f };
	glm::mat4 m_storedRotMat = glm::mat4(1.0f);

	friend class EntityEditorBlock;
};