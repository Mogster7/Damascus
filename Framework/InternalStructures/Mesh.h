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
	struct MeshData
    {
		std::vector<VertexType> vertices;
		std::vector<uint32_t> indices;
    };

    void CreateStatic(
		const std::vector<VertexType>& vertices, 
        const std::vector<uint32_t>& indices,
		Device& device)
    {
		vertexBuffer.CreateStatic(vertices, device);
		indexBuffer.CreateStatic(indices, device);
	}

    void CreateStatic(
		const std::vector<VertexType>& vertices, 
		Device& device)
    {
		vertexBuffer.CreateStatic(vertices, device);
	}

	// User is responsible for updating / staging
    void CreateDynamic(
		std::vector<VertexType>& vertices,
        std::vector<uint32_t>& indices,
		Device& device)
    {
		vertexBuffer.CreateDynamic(vertices, device);
		indexBuffer.CreateDynamic(indices, device);
		dynamic = true;
	}

    void CreateDynamic(
		std::vector<VertexType>& vertices,
		Device& device)
    {
		vertexBuffer.CreateDynamic(vertices, device);
		dynamic = true;
	}


	void UpdateDynamic(
		std::vector<VertexType>& vertices,
		std::vector<uint32_t>& indices)
	{
		vertexBuffer.UpdateData(vertices.data(), vertices.size() * sizeof(VertexType), vertices.size(), false);
		indexBuffer.UpdateData(vertices.data(), vertices.size() * sizeof(uint32_t), indices.size(), false);
	}

	void UpdateDynamic(std::vector<VertexType>& vertices)
	{
		vertexBuffer.UpdateData(vertices.data(), vertices.size() * sizeof(VertexType), vertices.size(), false);
	}


	template <class T> 
	std::vector<T> GetVertexBufferData(uint32_t offset)
	{
		std::vector<T> data;
		auto* buffer = reinterpret_cast<char*>(vertexBuffer.GetMappedData());
		for (int i = 0; i < vertexBuffer.GetVertexCount(); ++i)
		{
			data.emplace_back(
				*reinterpret_cast<T*>(buffer + offset + sizeof(VertexType) * i)
			);
		}

		return data;
	}

	void StageDynamic(vk::CommandBuffer commandBuffer)
	{
		vertexBuffer.StageTransferDynamic(commandBuffer);
		if (indexBuffer.GetIndexCount() > 0)
			indexBuffer.StageTransferDynamic(commandBuffer);
	}


	static MeshData LoadModel(const std::string& path)
    {
        ASSERT(false, "There is no template specialization for this model creation.");
    }
    void CreateModel(const std::string& path, bool dynamic, Device& owner)
    {
        ASSERT(false, "There is no template specialization for this model creation.");
		
    }
	
    void Destroy()
    {
		if (indexBuffer.GetIndexCount() > 0)
				indexBuffer.Destroy();

		vertexBuffer.Destroy();
    }

    void SetModel(const glm::mat4& model) { this->model = model; }

    uint32_t GetIndexCount() const { return indexBuffer.GetIndexCount(); }
    const Buffer& GetIndexBuffer() const { return indexBuffer; }
    void DestroyIndexBuffer() { indexBuffer.Destroy(); }

    uint32_t GetVertexCount() const { return vertexBuffer.GetVertexCount(); }
    const Buffer& GetVertexBuffer() const { return vertexBuffer; }
    void DestroyVertexBuffer() { vertexBuffer.Destroy(); }
	

	bool dynamic = false;
    glm::mat4 model = glm::mat4(1.0f);
    VertexBuffer<VertexType> vertexBuffer = {};
    IndexBuffer indexBuffer = {};
};

template<>
Mesh<Vertex>::MeshData Mesh<Vertex>::LoadModel(const std::string& path)
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
	bool hasTextureCoords = !attrib.texcoords.empty();
	bool hasNormals = !attrib.normals.empty();
	bool hasColors = !attrib.colors.empty();
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

			if (hasTextureCoords)
			{
				vertex.texPos = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};
			}

			if (hasColors)
			{
				vertex.color =
				{
					attrib.colors[3 * index.vertex_index + 0],
					attrib.colors[3 * index.vertex_index + 1],
					attrib.colors[3 * index.vertex_index + 2]
				};
			}

			if (hasNormals)
			{
				vertex.normal =
				{
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};
			}

			if (uniqueVerts.count(vertex) == 0) {
				uniqueVerts[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVerts[vertex]);
		}
	}

	return { std::move(vertices), std::move(indices) };
}

template <>
void Mesh<Vertex>::CreateModel(const std::string& path, bool dynamic, Device& owner)
{
	auto data = LoadModel(path);
	if (dynamic)
		CreateDynamic(data.vertices, data.indices, owner);
	else
		CreateStatic(data.vertices, data.indices, owner);
}


