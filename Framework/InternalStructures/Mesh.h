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
	static Mesh<VertexType> UnitSphere;
	static Mesh<VertexType> UnitCube;
	Mesh() = default;
	~Mesh() = default;

	static void InitializeStatics(Device& owner)
	{
		std::vector<Vertex> vertices;

		int i;
#define Band_Power  8  // 2^Band_Power = Total Points in a band.
#define Band_Points 256 // 16 = 2^Band_Power
#define Band_Mask (Band_Points-2)
#define Sections_In_Band ((Band_Points/2)-1)
#define Total_Points (Sections_In_Band*Band_Points) 
		// remember - for each section in a band, we have a band
#define Section_Arc (6.28/Sections_In_Band)
		const float R = -1.0; // radius of 10

		float x_angle;
		float y_angle;

		for (i = 0; i < Total_Points; i++)
		{
			// using last bit to alternate,+band number (which band)
			x_angle = (float)(i & 1) + (i >> Band_Power);

			// (i&Band_Mask)>>1 == Local Y value in the band
			// (i>>Band_Power)*((Band_Points/2)-1) == how many bands
			//  have we processed?
			// Remember - we go "right" one value for every 2 points.
			//  i>>bandpower - tells us our band number
			y_angle = (float)((i & Band_Mask) >> 1) + ((i >> Band_Power) * (Sections_In_Band));

			x_angle *= (float)Section_Arc / 2.0f; // remember - 180° x rot not 360
			y_angle *= (float)Section_Arc * -1;


			Vertex vertex;
			vertex.pos = {
					R * sin(x_angle) * sin(y_angle),
				   R * cos(x_angle),
				   R * sin(x_angle) * cos(y_angle)
			};
			vertices.emplace_back(vertex);
		}
		UnitSphere.CreateStatic(vertices, owner);


		std::vector<Vertex> cubeVerts = {
		{ { -1.0, -1.0f, 1.0f }, { 1.0f, 0.0f, 1.0f } , { 1.0f, 0.0f, 1.0f }},
		{ { 1.0f, -1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } , { 0.0f, 0.0f, 1.0f }},

		{ { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 1.0f } , { 0.0f, 1.0f, 1.0f } },
		{ { -1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 1.0f } , { 1.0f, 0.0f, 1.0f }},

		{ { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f  }, { 0.0f, 0.0f, 0.0f}},
		{ { 1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f  }},

		{ { 1.0f, 1.0f, -1.0f }, { 1.0f, 1.0f, 0.0f } , { 1.0f, 1.0f, 0.0f }},
		{ { -1.0f, 1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f  }},
		};


		const std::vector<uint32_t> cubeIndices =
		{
			// front
			0, 1, 2,
			2, 3, 0,
			// right
			1, 5, 6,
			6, 2, 1,
			// back
			7, 6, 5,
			5, 4, 7,
			// left
			4, 0, 3,
			3, 7, 4,
			// bottom
			4, 5, 1,
			1, 0, 4,
			// top
			3, 2, 6,
			6, 7, 3
		};

		for (auto& vert : cubeVerts)
			vert.normal = glm::normalize(vert.pos);

		UnitCube.CreateStatic(cubeVerts, cubeIndices, owner);

		auto cubePositions2 = Mesh<Vertex>::UnitCube.GetVertexBufferData<glm::vec3>(0);
		auto cubeIndices2 = Mesh<Vertex>::UnitCube.GetIndexBufferData();

		std::vector<PosVertex> cubePosVertices(cubePositions2.begin(), cubePositions2.end());

		auto spherePositions2 = Mesh<Vertex>::UnitSphere.GetVertexBufferData<glm::vec3>(0);
		std::vector<PosVertex> spherePosVertices(spherePositions2.begin(), spherePositions2.end());

		Mesh<PosVertex>::UnitCube.CreateStatic(cubePosVertices, cubeIndices2, owner);
		Mesh<PosVertex>::UnitSphere.CreateStatic(spherePosVertices, owner);
	}

	struct MeshData
    {
		std::vector<VertexType> vertices;
		std::vector<uint32_t> indices;
    };

	void Create(Mesh<VertexType>& other)
	{
		Mesh<VertexType> mesh;
		uint32_t indexCount = other.GetIndexCount();

		if (other.dynamic)
		{
			if (indexCount > 0)
				CreateDynamic(other.GetVertexBufferData<VertexType>(0),
							  other.GetIndexBufferData(), *other.m_owner);
			else
				CreateDynamic(other.GetVertexBufferData<VertexType>(0),
							  *other.m_owner);
		}
		else
		{
			if (indexCount > 0)
				CreateStatic(other.GetVertexBufferData<VertexType>(0),
							 other.GetIndexBufferData(), *other.m_owner);
			else
				CreateStatic(other.GetVertexBufferData<VertexType>(0),
							  *other.m_owner);
		}

	}


    void CreateStatic(
		const std::vector<VertexType>& vertices, 
        const std::vector<uint32_t>& indices,
		Device& device)
    {
		m_owner = &device;
		vertexBuffer.CreateStatic(vertices, device);
		indexBuffer.CreateStatic(indices, device);
	}

    void CreateStatic(
		const std::vector<VertexType>& vertices, 
		Device& device)
    {
		m_owner = &device;
		vertexBuffer.CreateStatic(vertices, device);
	}

	// User is responsible for updating / staging
    void CreateDynamic(
		std::vector<VertexType>& vertices,
        std::vector<uint32_t>& indices,
		Device& device)
    {
		m_owner = &device;
		vertexBuffer.CreateDynamic(vertices, device);
		indexBuffer.CreateDynamic(indices, device);
		dynamic = true;
	}

    void CreateDynamic(
		std::vector<VertexType>& vertices,
		Device& device)
    {
		m_owner = &device;
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
	std::vector<T> GetVertexBufferData(uint32_t offset) const
	{
		std::vector<T> data;
		const auto* buffer = reinterpret_cast<const char*>(vertexBuffer.GetMappedData());
		for (int i = 0; i < vertexBuffer.GetVertexCount(); ++i)
		{
			data.emplace_back(
				*reinterpret_cast<const T*>(buffer + offset + sizeof(VertexType) * i)
			);
		}

		return std::move(data);
	}

	std::vector<uint32_t> GetIndexBufferData() const
	{
		std::vector<uint32_t> data;
		const auto* buffer = reinterpret_cast<const char*>(indexBuffer.GetMappedData());
		for (int i = 0; i < indexBuffer.GetIndexCount(); ++i)
		{
			data.emplace_back(
				*reinterpret_cast<const uint32_t*>(buffer + sizeof(uint32_t) * i)
			);
		}

		return std::move(data);
	}



	VertexType* GetVertexBufferData()
	{
		return reinterpret_cast<VertexType*>(vertexBuffer.GetMappedData());
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

	void Bind(vk::CommandBuffer commandBuffer) const
	{
		vk::DeviceSize offset = 0;
		commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer.Get(), &offset);
		bool hasIndex = GetIndexCount() > 0;
		if (hasIndex)
			commandBuffer.bindIndexBuffer(GetIndexBuffer(), 0, vk::IndexType::eUint32);
	}

	void Draw(vk::CommandBuffer commandBuffer) const
	{
		bool hasIndex = GetIndexCount() > 0;
		hasIndex ?
			commandBuffer.drawIndexed(GetIndexCount(), 1, 0, 0, 0)
			:
			commandBuffer.draw(GetVertexCount(), 1, 0, 0);
	}

    void Destroy()
    {
		if (m_owner == nullptr) return;

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
    VertexBuffer<VertexType> vertexBuffer = {};
    IndexBuffer indexBuffer = {};

	private:
		Device* m_owner = nullptr;
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


