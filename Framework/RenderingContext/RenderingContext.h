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
//DM_TYPE(RenderingContext)
class RenderingContext : public DeviceContext
{
public:
	// Must be defined by a compilation unit for any application
	static RenderingContext& Get();

	[[nodiscard]] size_t GetImageViewCount() const
	{ return swapchain.imageViews.size(); }

	virtual void Create(std::weak_ptr<Window> window);

	// Draw
	virtual void Draw() = 0;

	static float UpdateDeltaTime();
	// Update
	virtual void Update(float dt);
	virtual void Destroy();

	Swapchain swapchain = {};
	CommandPool commandPool;

	// Descriptors for forward pipeline
	Descriptors descriptors = {};
	vk::PushConstantRange pushRange = {};
	std::vector<Buffer> uniformBufferViewProjection = {};

	struct ForwardPass : public PipelinePass
	{
		FrameBufferAttachment depth;
	} forwardPass;
	// ---------------

	std::vector <FrameBuffer> frameBuffers = {};
	std::vector <CommandBuffer> drawBuffers = {};
	std::vector <Semaphore> imageAvailable = {};
	std::vector <Semaphore> renderFinished = {};
	std::vector <Fence> drawFences = {};
	RenderPass renderPass = {};
	FrameBufferAttachment depth;

	// Values for tracking current async state
	uint32_t currentFrame = 0;
	uint32_t imageIndex = -1;

	std::weak_ptr <Window> window;

	// Default values
	std::array<float, 4> defaultClearColor = { 0.2f, 0.2f, 0.2f, 1.0f };
	float defaultClearDepth = 1.0f;

	// Return if surface is out of date
	bool PrepareFrame(uint32_t frameIndex);
	bool SubmitFrame(const vk::Semaphore *wait, uint32_t waitCount);

protected:
	void CreateLogicalDevice();
	void CreateSwapchain(bool recreate = false);
	void CreateRenderPass();
	void CreateForwardPipeline();
	void CreateSync();
	void CreateFramebuffers(bool recreate = false);
	void CreateDepthBuffer(bool recreate = false);
	void CreateCommandBuffers(bool recreate = false);
	void CreateCommandPool();
};

}