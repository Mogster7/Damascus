#pragma once
struct RenderingContext_Impl
{    
    RenderingContext_Impl() = default;
    ~RenderingContext_Impl();
    
    // Draw
    void DrawFrame();

     // Update
    virtual void Update(float dt);

    
    // Init functions
    void InitVulkan(std::weak_ptr<Window> winHandle);

    Instance instance = {};
    PhysicalDevice physicalDevice = {};
    Device device = {};

    uint32_t currentFrame = 0;

    std::weak_ptr<Window> window;
};

inline void RenderingContext_Impl::InitVulkan(std::weak_ptr<Window> winHandle) {
    this->window = winHandle;
    instance.Create();
    instance.CreateSurface(winHandle);
    physicalDevice.Create();
    physicalDevice.CreateLogicalDevice(device);
}

inline RenderingContext_Impl::~RenderingContext_Impl()
{
    device.Get().waitIdle();

    device.Destroy();
    instance.Destroy();
}

inline void RenderingContext_Impl::DrawFrame()
{
    device.DrawFrame(currentFrame);
    currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
}

inline void RenderingContext_Impl::Update(float dt)
{
    device.Update(dt);
}

