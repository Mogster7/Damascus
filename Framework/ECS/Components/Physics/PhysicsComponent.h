#pragma once

class PhysicsComponent
{
public:
	glm::vec3 velocity = glm::vec3(0.0f);

private:

	friend class EntityEditorBlock;
};
