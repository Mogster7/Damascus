//------------------------------------------------------------------------------
//
// File Name:	Mesh.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/8/2020
//
//------------------------------------------------------------------------------
#include "RenderingStructures.hpp"

Mesh::Mesh(vk::PhysicalDevice physDevice, vk::Device device, const std::vector<Vertex>& vertices) :
    m_VertexCount(vertices.size()), 
    m_PhysicalDevice(physDevice),
    m_Device(device)
{
    CreateVertexBuffer(vertices);
}

int Mesh::GetVertexCount() const
{
    return m_VertexCount;
}

vk::Buffer Mesh::GetVertexBuffer() const
{
    return m_VertexBuffer;
}

void Mesh::DestroyVertexBuffer()
{
    m_Device.freeMemory(m_VertexBufferMemory);
    m_Device.destroyBuffer(m_VertexBuffer);
}

void Mesh::CreateVertexBuffer(const std::vector<Vertex>& vertices)
{
    // Create vertex buffer
    vk::BufferCreateInfo createInfo;
    createInfo.size = sizeof(Vertex) * vertices.size();
    createInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
    createInfo.sharingMode = vk::SharingMode::eExclusive;

    utils::CheckVkResult(m_Device.createBuffer(&createInfo, nullptr, &m_VertexBuffer), 
        "Failed to create vertex buffer");

    // Get buffer memory requirements
    vk::MemoryRequirements memReq;
    m_Device.getBufferMemoryRequirements(m_VertexBuffer, &memReq);

    // Allocate memory
    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memReq.size;
    // Provide index of memory type on physical device that has the required bits
    // Memory is visible to the host, and allows placement of data straight into buffer after mapping
    // (No flushing required)
    allocInfo.memoryTypeIndex = FindMemoryTypeIndex(memReq.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eHostVisible |
        vk::MemoryPropertyFlagBits::eHostCoherent
    );

    // Allocate memory to device memory
    utils::CheckVkResult(m_Device.allocateMemory(&allocInfo, nullptr, &m_VertexBufferMemory),
        "Failed to allocate vertex buffer memory");

    // Allocate memory to the vertex buffer
    m_Device.bindBufferMemory(m_VertexBuffer, m_VertexBufferMemory, 0);
    
    // Map memory to the vertex buffer

    // Create pointer to memory
    void* data; 

    // Map and copy vertices to the memory, then unmap
    m_Device.mapMemory(m_VertexBufferMemory, 0, createInfo.size, {}, &data);
    std::memcpy(data, vertices.data(), (size_t)createInfo.size);
    m_Device.unmapMemory(m_VertexBufferMemory);
}

uint32_t Mesh::FindMemoryTypeIndex(uint32_t allowedTypes, vk::MemoryPropertyFlags properties)
{
    // Get properties of physical device memory
    vk::PhysicalDeviceMemoryProperties memProps;
    m_PhysicalDevice.getMemoryProperties(&memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
    {
        // Index of memory type must match corresponding bit in allowed types
        // And that the property types are valid by checking if the result
        // of propertyFlags and properties give us the original property field back
        // In other words, the propertyFlags allowed either match or exceed the number
        // of properties described by the local variable 'properties'
        if ((allowedTypes & (1 << i))
            && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
        {
            // Memory is valid, return index
            return i;
        }

    }

    throw std::runtime_error("Failed to find a correct physical device memory index for our flags");
}
