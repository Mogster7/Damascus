//------------------------------------------------------------------------------
//
// File Name:	Mesh.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/8/2020 
//
//------------------------------------------------------------------------------
#pragma once

#include "glm/gtx/hash.hpp"
#include "tiny/tiny_obj_loader.h"

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texPos) << 1);
        }
    };
}

template <class VertexType>
class Mesh
{
public:
    void Create(Device& device, const std::vector<VertexType>& vertices, 
        const std::vector<uint32_t>& indices)
    {
		m_vertexBuffer.Create(vertices, device);

		if (!indices.empty())
			m_indexBuffer.Create(indices, device);
    }

    void CreateModel(std::string path, Device& owner)
    {
        ASSERT(false, "There is no template specialization for this model creation.");
    }
	
    void Destroy()
    {
		if (m_indexBuffer.GetIndexCount() > 0)
				m_indexBuffer.Destroy();

		m_vertexBuffer.Destroy();
    }

    void SetModel(const glm::mat4& model) { m_model = model; }
    const glm::mat4& GetModel() const { return m_model; }

    uint32_t GetIndexCount() const { return m_indexBuffer.GetIndexCount(); }
    const Buffer& GetIndexBuffer() const { return m_indexBuffer; }
    void DestroyIndexBuffer() { m_indexBuffer.Destroy(); }

    uint32_t GetVertexCount() const { return m_vertexBuffer.GetVertexCount(); }
    const Buffer& GetVertexBuffer() const { return m_vertexBuffer; }
    void DestroyVertexBuffer() { m_vertexBuffer.Destroy(); }

private:
    glm::mat4 m_model = glm::mat4(1.0f);

    VertexBuffer<VertexType> m_vertexBuffer = {};
    IndexBuffer m_indexBuffer = {};

};

template <>
void Mesh<Vertex>::CreateModel(std::string path, Device& owner)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, error;

	ASSERT(
		tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &error, path.c_str()),
		("Failed to load model at " + path).c_str()
	);

	std::unordered_map<Vertex, uint32_t> uniqueVerts;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};

			vertex.pos = {
	attrib.vertices[3 * index.vertex_index + 0],
	attrib.vertices[3 * index.vertex_index + 1],
	attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texPos = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };

			if (uniqueVerts.count(vertex) == 0) {
				uniqueVerts[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVerts[vertex]);
		}
	}

	Create(owner, vertices, indices);
}


