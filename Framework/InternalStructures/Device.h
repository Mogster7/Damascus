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
    bool SubmitFrame(const uint32_t frameIndex, const vk::Semaphore* wait, uint32_t waitCount);

    void DrawFrame(const uint32_t frameIndex);
    void Initialization();

    VmaAllocator allocator = {};
    vk::Queue graphicsQueue = {};
    vk::Queue presentQueue = {};

const PhysicalDevice& GetPhysicalDevice() const {
        return *m_owner;
    }

    const Instance& GetInstance() const {
        return m_owner->GetInstance();
    }

private:
    void CreateAllocator();
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




