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
    void DrawFrame(const uint32_t frameIndex);
    void Initialization();

    static const std::vector<Vertex> Device::meshVerts;
    static const std::vector<Vertex> Device::meshVerts2;

    static const std::vector<uint32_t> Device::meshIndices;
    std::vector<Mesh> objects;

    VmaAllocator allocator = {};
    Swapchain swapchain = {};

    vk::Queue graphicsQueue = {};
    vk::Queue presentQueue = {};

    RenderPass renderPass = {};

    PipelineLayout pipelineLayout = {};
    GraphicsPipeline graphicsPipeline = {};

    std::vector<Framebuffer> framebuffers = {};

    CommandPool commandPool = {};
    std::vector<vk::CommandBuffer> commandBuffers;

    std::vector<Semaphore> imageAvailable = {};
    std::vector<Semaphore> renderFinished = {};
    std::vector<Fence> drawFences = {};

    // Descriptors
    DescriptorSetLayout descriptorSetLayout;
    vk::PushConstantRange pushRange;

    DescriptorPool descriptorPool;
    std::vector<vk::DescriptorSet> descriptorSets;

    std::vector<Buffer> uniformBufferModel;
    std::vector<Buffer> uniformBufferViewProjection;

    struct UboViewProjection
    {
        glm::mat4 projection;
        glm::mat4 view;
    } uboViewProjection;

    DepthBuffer depthBuffer;

    //size_t modelUniformAlignment;
    //vk::DeviceSize minUniformBufferOffset;
    //Model* modelTransferSpace;


    void AllocateDynamicBufferTransferSpace();

private:
    void CreateAllocator();
    void CreateSwapchain();
    void CreateRenderPass();
    void CreateDescriptorSetLayout();
    void CreatePushConstantRange();
    void CreateGraphicsPipeline();
    void CreateDepthBufferImage();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSync();
    void RecordCommandBuffers(uint32_t imageIndex);

    void CreateUniformBuffers();
    void CreateDescriptorPool();
    void CreateDescriptorSets();

    void UpdateUniformBuffers(uint32_t imageIndex);
    void UpdateModel(glm::mat4& newModel);
};




