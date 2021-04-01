#pragma once

class ComponentSystem
{

public:
	virtual void Create() {};
	virtual void Update(float dt) = 0;
	virtual void Destroy() {};
};