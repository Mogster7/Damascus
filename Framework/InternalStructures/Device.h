//------------------------------------------------------------------------------
//
// File Name:	Device.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/18/2020 
//
//------------------------------------------------------------------------------
#pragma once

CUSTOM_VK_DECLARE_DERIVE(Device, Device, PhysicalDevice)


public:
    void Update(float dt);
    
    // Return whether or not the surface is out of date
    bool PrepareFrame(const uint32_t frameIndex);
    bool SubmitFrame(const uint32_t frameIndex, vk::Semaphore wait);

    void DrawFrame(const uint32_t frameIndex);
    void Initialization();

    uint32_t imageIndex;

    VmaAllocator allocator = {};
    Swapchain swapchain = {};

    vk::Queue graphicsQueue = {};
    vk::Queue presentQueue = {};

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

    //size_t modelUniformAlignment;
    //vk::DeviceSize minUniformBufferOffset;
    //Model* modelTransferSpace;


    void AllocateDynamicBufferTransferSpace();
    const PhysicalDevice& GetPhysicalDevice() const {
        return *m_owner;
    }

    const Instance& GetInstance() const {
        return m_owner->GetInstance();
    }

private:
    void CreateAllocator();
    void CreateSwapchain(bool recreate = false);
    void CreateRenderPass();
    void CreateDescriptorSetLayout();
    void CreatePushConstantRange();
    void CreateGraphicsPipeline();
    void CreateDepthBuffer(bool recreate = false);
    void CreateFramebuffers(bool recreate = false);
    void CreateCommandPool();
    void CreateCommandBuffers(bool recreate = false);
    void CreateSync();
    void RecordCommandBuffers(uint32_t imageIndex);

    void CreateUniformBuffers();
    void CreateDescriptorPool();
    void CreateDescriptorSets();

    void UpdateUniformBuffers(uint32_t imageIndex);
    void UpdateModel(glm::mat4& newModel);

    void RecreateSurface();
};




