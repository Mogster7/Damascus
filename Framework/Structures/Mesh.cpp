//------------------------------------------------------------------------------
//
// File Name:	Mesh.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/8/2020
//
//------------------------------------------------------------------------------
#include "RenderingStructures.hpp"

Mesh::Mesh(vk::PhysicalDevice physDevice, vk::Device device,
    const std::vector<Vertex>& vertices) :
    m_VertexBuffer(physDevice, device, vertices)
{

}
