//------------------------------------------------------------------------------
//
// File Name:	RenderingContext.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/5/2020 
//
//------------------------------------------------------------------------------
#pragma once

class Overlay;

class RenderingContext
{
public:
    RenderingContext() = default;
    ~RenderingContext() = default;

    // Must be defined by a compilation unit for each demo
    static RenderingContext& Get();
    
    // Draw
    virtual void Draw() = 0;

     // Update
    virtual void Update();

    // Init functions
    virtual void Initialize(std::weak_ptr<Window> winHandle, bool enabledOverlay = true);

    virtual void Destroy();


    Instance instance = {};
    PhysicalDevice physicalDevice = {};
    Device device = {};
    std::unique_ptr<Overlay> overlay = {};
    bool enabledOverlay = false;


    Swapchain swapchain = {};

    RenderPass renderPass = {};

    PipelineLayout pipelineLayout = {};
    GraphicsPipeline pipeline = {};

    std::vector<FrameBuffer> frameBuffers = {};
    std::vector<CommandBuffer> drawBuffers = {};

    CommandPool commandPool = {};

    std::vector<Semaphore> imageAvailable = {};
    std::vector<Semaphore> renderFinished = {};
    std::vector<Fence> drawFences = {};

    FrameBufferAttachment depth;

    uint32_t currentFrame = 0;
    uint32_t imageIndex;

    float dt = 0.0f;
    std::weak_ptr<Window> window;

	bool PrepareFrame(const uint32_t frameIndex);
	// Return if surface is out of date
	bool SubmitFrame(const uint32_t frameIndex, const vk::Semaphore* wait, uint32_t waitCount);


protected:
	void CreateSwapchain(bool recreate = false);
	void CreateRenderPass();
	void CreateSync();
	void CreateFramebuffers(bool recreate = false);
	void CreateDepthBuffer(bool recreate = false);
	void CreateCommandBuffers(bool recreate);
	void CreateCommandPool();


};





