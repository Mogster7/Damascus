#include "Object.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/ext/matrix_clip_space.hpp"

//
//void Object::SetPosition(const glm::vec3& position)
//{
//	if (position == this->m_position) return;
//
//	this->m_position = position;
//	dirty = true;
//}
//
//void Object::SetScale(const glm::vec3& scale)
//{
//	if (scale == this->m_scale) return;
//	this->m_scale = scale;
//	dirty = true;
//}
//
//void Object::SetRotation(const glm::vec3& rotation)
//{
//	if (rotation == this->m_rotation) return;
//	this->m_rotation = rotation;
//	dirty = true;
//}
//
//
//void Object::UpdateModel()
//{
//	if (!dirty) return;
//
//
//	glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), m_scale);
//	glm::vec3 rot = glm::radians(m_rotation);
//	glm::quat pitch = glm::angleAxis(rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
//	glm::quat yaw = glm::angleAxis(rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
//	glm::quat roll = glm::angleAxis(rot.z, glm::vec3(0.0f, 0.0f, 1.0f));
//
//	glm::quat orientation = glm::normalize(pitch * yaw * roll);
//	m_storedRotMat = glm::mat4_cast(orientation);
//
//	glm::mat4 translateMat = glm::translate(glm::mat4(1.0f), m_position);
//
//	model = translateMat * m_storedRotMat * scaleMat;
//
//	if (collider)
//		collider->Update(
//			mesh,
//			m_position,
//			m_storedRotMat,
//			m_scale);
//
//	dirty = false;
//}
//
//void Object::Destroy()
//{
//	mesh.Destroy();
//}
