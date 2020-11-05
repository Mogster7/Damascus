//------------------------------------------------------------------------------
//
// File Name:	Mesh.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/8/2020
//
//------------------------------------------------------------------------------
#include "RenderingStructures.hpp"

void Mesh::Create(Device& device, const eastl::vector<Vertex>& vertices, 
    const eastl::vector<uint32_t>& indices)
{
    m_VertexBuffer.Create(vertices, device, device.GetMemoryAllocator());

    if (!indices.empty())
        m_IndexBuffer.Create(indices, device, device.GetMemoryAllocator());
}

void Mesh::Destroy()
{
    if (m_IndexBuffer.GetIndexCount() > 0)
        m_IndexBuffer.Destroy();

    m_VertexBuffer.Destroy();
}
