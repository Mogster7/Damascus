
#pragma once
#include "RenderingContext.h"

class DeferredRenderingContext_Impl;

class DeferredRenderingContext : public RenderingContext
{
	static DeferredRenderingContext_Impl* impl;

public:
	static DeferredRenderingContext_Impl* GetImplementation() { return impl; }
	static void Initialize(std::weak_ptr<Window> window);
	static void Update();
	static void DrawFrame();
};

