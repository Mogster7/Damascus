//------------------------------------------------------------------------------
//
// File Name:	Buffer.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/9/2020 
//
//------------------------------------------------------------------------------
#pragma once

class Buffer
{
public:
    Buffer() = default;
    Buffer(vk::PhysicalDevice physDevice, vk::Device device, vk::DeviceSize bufferSize,
        vk::MemoryPropertyFlags bufferProps, vk::BufferUsageFlags bufferUsage, 
        vk::SharingMode sharingMode = vk::SharingMode::eExclusive);

    ~Buffer() = default;

    vk::Buffer GetBuffer() const { return m_Buffer; }
    vk::DeviceMemory GetMemory() const { return m_Memory; }

    static void StageTransfer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);

    void Destroy();


protected:
    uint32_t FindMemoryTypeIndex(uint32_t allowedTypes) const;

    vk::Buffer m_Buffer = {};
    vk::PhysicalDevice m_PhysicalDevice = {};
    vk::Device m_Device = {};
    vk::MemoryPropertyFlags m_Properties = {};
    vk::BufferUsageFlags m_Usage = {};
    vk::SharingMode m_SharingMode = {};
    vk::DeviceSize m_Size = {};
    vk::DeviceMemory m_Memory = {};

};



class VertexBuffer : public Buffer
{
public:
    VertexBuffer() = default;
    VertexBuffer(vk::PhysicalDevice physDevice, vk::Device device, 
        const std::vector<Vertex>& vertices);
    ~VertexBuffer() = default;

    uint32_t GetVertexCount() const { return m_VertexCount; }

private:
    uint32_t m_VertexCount = 0;
};

