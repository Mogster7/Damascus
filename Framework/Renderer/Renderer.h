//------------------------------------------------------------------------------
//
// File Name:	RenderingContext.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/5/2020
//
//------------------------------------------------------------------------------
#pragma once
namespace dm
{

struct Renderer;

class IRenderingContext : public IOwned<Device>
{
protected:
    explicit IRenderingContext(Renderer& inRenderer);
    virtual void Create() {};
    virtual void OnRecreateSwapchain() = 0;
    virtual void Update(float dt){};
    virtual void AssignGlobalUniform(GlobalUniforms& globalUniforms) {}
    virtual std::vector<vk::SubmitInfo> Record() = 0;

    FrameAsync<Semaphore> ready;
    FrameAsync<Semaphore> finished;
    vk::PipelineStageFlags stageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;

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

struct BaseDeviceRenderer : public BaseRenderer
{
    Device device;
};

struct Renderer : public BaseDeviceRenderer
{
    void Create(std::weak_ptr<Window> inWindow);
    ~Renderer();

    template<class Context>
    Context* AddRenderingContext()
    {
        auto* context = new Context(*this);
        renderingContexts.emplace_back(typeid(Context), context).second->Create();
        return context;
    }

    template <class Context>
    void DeleteRenderingContext()
    {
        std::type_index type = typeid(Context);
        device.waitIdle();
        auto it = std::find_if(
            renderingContexts.begin(),
            renderingContexts.end(),
            [type](const std::pair<std::type_index, IRenderingContext*> & rc) { return rc.first == type; });

        DM_ASSERT(it != renderingContexts.end());

        delete it->second;
        renderingContexts.erase(it);
    }

    template <class Context>
    Context& GetRenderingContext()
    {
        std::type_index type = typeid(Context);
        auto it = std::find_if(
            renderingContexts.begin(),
            renderingContexts.end(),
            [type](const std::pair<std::type_index, IRenderingContext*> & rc) { return rc.first == type; });

        DM_ASSERT(it != renderingContexts.end());
        return *static_cast<Context*>(it->second);
    }

    [[nodiscard]] int ImageCount() const;

    CommandPool commandPool;
    DescriptorPool descriptorPool;
    Descriptors descriptors;
    CommandBufferVector commandBuffers;

    // Swapchain objects
    std::shared_ptr<Swapchain> swapchain = nullptr;     //< Current swapchain
    std::shared_ptr<Swapchain> oldSwapchain = nullptr;  //< Retired swapchain after swapchain reconstruction

    // Sync resources GPU-GPU
    std::vector<Semaphore> imageAvailable;  //< Semaphore signaling swapchain availability for the requested image.
    std::vector<Semaphore> renderFinished;  //< Semaphore signalling end of the render phase, so presentation may occur .

    // Sync resources CPU-GPU
    std::vector<Fence> frameResourcesInUse; //< Used to determine if frame sync resources is in use by GPU.
    std::vector<Fence> imagesInUse;         //< Used to determine if swapchain image is in use by GPU.

    int frameIndex = 0; //< Current frame we are submitting info to.
    int imageIndex = 0; //< Current image acquired from the swapchain.

    std::vector<std::pair<std::type_index, IRenderingContext*>> renderingContexts;

    bool framebufferResize = false; //< Flag to tell the renderer that window was resized.

    void Update(float dt);
    void Render();

private:
    bool created = false;

    void CreateDevice();
    void CreateSwapchain();
    void RecreateSwapchain();
    void CreateSync();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateDescriptorPool();

    bool PrepareFrame();
    void SubmitFrame(unsigned submitInfoCount, vk::SubmitInfo* submitInfo, vk::Fence fence) const;
    bool PresentFrame();
};



}// namespace dm