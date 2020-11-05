//------------------------------------------------------------------------------
//
// File Name:	Device.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/18/2020 
//
//------------------------------------------------------------------------------
#pragma once

CUSTOM_VK_DECLARE_DERIVE(Device, Device, PhysicalDevice)

    VmaAllocator m_Allocator = {};
    Swapchain m_Swapchain = {};

    vk::Queue m_GraphicsQueue = {};
    vk::Queue m_PresentQueue = {};

    RenderPass m_RenderPass = {};

    PipelineLayout m_PipelineLayout = {};
    GraphicsPipeline m_GraphicsPipeline = {};

    eastl::vector<Framebuffer> m_Framebuffers = {};

    CommandPool m_GraphicsCmdPool = {};
    eastl::vector<vk::CommandBuffer> m_CommandBuffers;

    eastl::vector<Semaphore> m_ImageAvailable = {};
    eastl::vector<Semaphore> m_RenderFinished = {};
    eastl::vector<Fence> m_DrawFences = {};

public:
    void Update(float dt);
    void DrawFrame(const uint32_t frameIndex);
    void Initialization();

    static const eastl::vector<Vertex> Device::meshVerts;
    static const eastl::vector<Vertex> Device::meshVerts2;

    static const eastl::vector<uint32_t> Device::meshIndices;
    eastl::vector<Mesh> objects;

    // Descriptors
    DescriptorSetLayout m_DescriptorSetLayout;
    vk::PushConstantRange m_PushRange;

    DescriptorPool m_DescriptorPool;
    eastl::vector<vk::DescriptorSet> m_DescriptorSets;

    eastl::vector<Buffer> m_UniformBufferModel;
    eastl::vector<Buffer> m_UniformBufferViewProjection;

    struct UboViewProjection
    {
        glm::mat4 projection;
        glm::mat4 view;
    } m_UboViewProjection;

    DepthBuffer m_DepthBuffer;

    //size_t m_ModelUniformAlignment;
    //vk::DeviceSize m_MinUniformBufferOffset;
    //Model* m_ModelTransferSpace;

    

    VmaAllocator GetMemoryAllocator() const { return m_Allocator; }
    Swapchain& GetSwapchain() { return m_Swapchain; }
    vk::Queue GetGraphicsQueue() const { return m_GraphicsQueue; }
    vk::Queue GetPresentQueue() const { return m_PresentQueue; }
    eastl::vector<Framebuffer>& GetFramebuffers() { return m_Framebuffers; }
    CommandPool& GetGraphicsCmdPool() { return m_GraphicsCmdPool; }
    eastl::vector<vk::CommandBuffer>& GetCommandBuffers() { return m_CommandBuffers; }
    eastl::vector<Fence>& GetDrawFences() { return m_DrawFences; }
    eastl::vector<Semaphore>& GetImageAvailableSemaphores() { return m_ImageAvailable; }
    eastl::vector<Semaphore>& GetRenderFinishedSemaphores() { return m_RenderFinished; }

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




