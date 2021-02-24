#pragma once
#include "Overlay/EditorBlock.h"
#include "Object/Object.h"

class ObjectEditorBlock : public EditorBlock
{
public:
	ObjectEditorBlock(std::vector<Object>& objects) : sceneObjects(objects) {}
	~ObjectEditorBlock();
	void Update(float dt) override;

	std::vector<Object>& sceneObjects;
};
