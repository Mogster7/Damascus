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
    bool SubmitFrame(const uint32_t frameIndex);

    void DrawFrame(const uint32_t frameIndex);
    void Initialization();

    uint32_t imageIndex;

    VmaAllocator allocator = {};
    Swapchain swapchain = {};

    vk::Queue graphicsQueue = {};
    vk::Queue presentQueue = {};

    RenderPass renderPass = {};

    PipelineLayout pipelineLayout = {};
    GraphicsPipeline graphicsPipeline = {};

    std::vector<Framebuffer> framebuffers = {};
    std::vector<vk::CommandBuffer> drawCmdBuffers;

    CommandPool commandPool = {};

    std::vector<Semaphore> imageAvailable = {};
    std::vector<Semaphore> renderFinished = {};
    std::vector<Fence> drawFences = {};


    DepthBuffer depthBuffer;

    //size_t modelUniformAlignment;
    //vk::DeviceSize minUniformBufferOffset;
    //Model* modelTransferSpace;


    void AllocateDynamicBufferTransferSpace();

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




