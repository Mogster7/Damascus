//------------------------------------------------------------------------------
//
// File Name:	RenderingContext.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/5/2020
//
//------------------------------------------------------------------------------
#pragma once
namespace dm {

struct Renderer;

class RenderingContext : public IOwned<Device>
{
protected:
    explicit RenderingContext(Renderer& inRenderer) : renderer(inRenderer) {}
    virtual void Update(float dt) {};
    virtual vk::CommandBuffer Record() = 0;

    Renderer& renderer;
    friend class Renderer;
};

/**
 * Base structures to handle cleaning up lower level structures below the logical device
 */
struct BaseRenderer
{
    Instance instance;
    PhysicalDevice physicalDevice;
};

struct Renderer : public BaseRenderer
{
    void Create(std::weak_ptr<Window> inWindow);
    ~Renderer();

    template<class Context>
    Context* AddRenderingContext()
    {
        auto* context = new Context(*this);
        renderingContexts.push_back(context);
        return context;
    }

    void DeleteRenderingContext(RenderingContext* context)
    {
        device.waitIdle();
        auto it = std::remove(renderingContexts.begin(), renderingContexts.end(), context);
        delete *it;
        renderingContexts.erase(it);
    }

    Device device;
    CommandPool commandPool;
    Swapchain swapchain;

    /** Semaphore signaling swapchain availability for the requested image */
    std::vector<Semaphore> imageAvailable;

    /** Semaphore signalling end of the render phase, so presentation may occur */
    std::vector<Semaphore> renderFinished;

    /** Used to determine when drawing is available */
    std::vector<Fence> drawFences;

    int frameIndex = 0;
    int imageIndex = 0;

    std::vector<RenderingContext*> renderingContexts;
    void Update(float dt);
    void Render();

private:
    bool created = false;

    void CreateDevice();
    void CreateSwapchain(bool recreate = false);
    void CreateSync();
    void CreateCommandPool();

    bool PrepareFrame();
    void SubmitFrame(unsigned submitInfoCount, vk::SubmitInfo* submitInfo, vk::Fence fence) const;
    bool PresentFrame();
};

}// namespace dm