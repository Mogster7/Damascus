#pragma once

class EditorBlock
{
public:
	EditorBlock() = default;

	virtual ~EditorBlock() = default;

	virtual void Update(float dt) = 0;

	std::vector<std::function<void(void)>> updateCallbacks;
};