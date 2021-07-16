//------------------------------------------------------------------------------
//
// File Name:	RenderingContext.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/5/2020 
//
//------------------------------------------------------------------------------
#pragma once
namespace dm {

class RenderingContext;

/**
 * Base structures to handle cleaning up lower level structures below the logical device
 */
struct Context
{
	Instance instance;
	PhysicalDevice physicalDevice;
};

struct DeviceContext : public Context
{
	Device device;
};


/**
 * Handles all of the rendering state, any application should extend and work with this class
 */
//BK_TYPE(RenderingContext)
class RenderingContext : public DeviceContext
{
public:

	// Must be defined by a compilation unit for each demo
	static RenderingContext& Get();

	virtual void Create(std::weak_ptr<Window> window);

	// Draw
	virtual void Draw() = 0;

	// Update
	virtual void Update();
	virtual void Destroy();

	Swapchain swapchain;

	RenderPass renderPass;

	PipelineLayout pipelineLayout;
	GraphicsPipeline pipeline;

	std::vector <FrameBuffer> frameBuffers = {};
	std::vector <CommandBuffer> drawBuffers = {};

	CommandPool commandPool;

	std::vector <Semaphore> imageAvailable = {};
	std::vector <Semaphore> renderFinished = {};
	std::vector <Fence> drawFences = {};

	FrameBufferAttachment depth;

	uint32_t currentFrame = 0;
	uint32_t imageIndex = -1;

	float dt = 0.0f;
	std::weak_ptr <Window> window;

	bool PrepareFrame(uint32_t frameIndex);

	// Return if surface is out of date
	bool SubmitFrame(uint32_t frameIndex, const vk::Semaphore *wait, uint32_t waitCount);


protected:
	void CreateLogicalDevice();

	void CreateSwapchain(bool recreate = false);

	void CreateRenderPass();

	void CreateSync();

	void CreateFramebuffers(bool recreate = false);

	void CreateDepthBuffer(bool recreate = false);

	void CreateCommandBuffers(bool recreate = false);

	void CreateCommandPool();
};

}