#pragma once

class EditorBlock
{
public:
	EditorBlock() = default;

	virtual ~EditorBlock() = default;

	virtual void Update(float dt) = 0;
};