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

    int GetVertexCount() const { return m_VertexBuffer.GetVertexCount(); }
    vk::Buffer GetVertexBuffer() const { return m_VertexBuffer.GetBuffer(); }
    void DestroyVertexBuffer() { m_VertexBuffer.Destroy(); }

private:
    VertexBuffer m_VertexBuffer = {};

};




