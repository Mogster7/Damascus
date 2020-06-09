//------------------------------------------------------------------------------
//
// File Name:	Mesh.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/8/2020 
//
//------------------------------------------------------------------------------
#pragma once

class Mesh
{
public:
    Mesh(vk::PhysicalDevice physDevice, vk::Device device, const std::vector<Vertex>& vertices);

    int GetVertexCount() const;
    vk::Buffer GetVertexBuffer() const;
    void DestroyVertexBuffer();

private:
    int m_VertexCount = {};
    vk::PhysicalDevice m_PhysicalDevice = {};
    vk::Device m_Device = {};
    vk::Buffer m_VertexBuffer = {};
    vk::DeviceMemory m_VertexBufferMemory;

    void CreateVertexBuffer(const std::vector<Vertex>& vertices);
    uint32_t FindMemoryTypeIndex(uint32_t allowedTypes, vk::MemoryPropertyFlags properties);
};




