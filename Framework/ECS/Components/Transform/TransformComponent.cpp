#include "TransformComponent.h"

void TransformComponent::SetPosition(const glm::vec3& position)
{
	if (position == this->m_position) return;

	this->m_position = position;
	dirty = true;
}

void TransformComponent::SetScale(const glm::vec3& scale)
{
	if (scale == this->m_scale) return;
	this->m_scale = scale;
	dirty = true;
}

void TransformComponent::SetRotation(const glm::vec3& rotation)
{
	if (rotation == this->m_rotation) return;
	this->m_rotation = rotation;
	dirty = true;
}

void TransformComponent::PushModel(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout) const
{
	commandBuffer.pushConstants(
		pipelineLayout,
		vk::ShaderStageFlagBits::eVertex,
		0, sizeof(glm::mat4), &model
	);
}

void TransformComponent::UpdateModel()
{
	if (!dirty) return;

	glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), m_scale);
	glm::vec3 rot = glm::radians(m_rotation);
	glm::quat pitch = glm::angleAxis(rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::quat yaw = glm::angleAxis(rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::quat roll = glm::angleAxis(rot.z, glm::vec3(0.0f, 0.0f, 1.0f));

	glm::quat orientation = glm::normalize(pitch * yaw * roll);
	m_storedRotMat = glm::mat4_cast(orientation);

	glm::mat4 translateMat = glm::translate(glm::mat4(1.0f), m_position);

	model = translateMat * m_storedRotMat * scaleMat;

	dirty = false;
}
