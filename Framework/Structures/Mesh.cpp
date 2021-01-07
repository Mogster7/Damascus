//------------------------------------------------------------------------------
//
// File Name:	Mesh.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/8/2020
//
//------------------------------------------------------------------------------
#include "RenderingStructures.hpp"

void Mesh::Create(Device& device, const std::vector<Vertex>& vertices, 
    const std::vector<uint32_t>& indices)
{
    m_vertexBuffer.Create(vertices, device, device.allocator);

    if (!indices.empty())
        m_indexBuffer.Create(indices, device, device.allocator);
}

void Mesh::Destroy()
{
    if (m_indexBuffer.GetIndexCount() > 0)
        m_indexBuffer.Destroy();

    m_vertexBuffer.Destroy();
}
