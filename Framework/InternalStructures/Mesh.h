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

//#include
namespace std {
template<>
struct hash<dm::Vertex>
{
	size_t operator()(dm::Vertex const& vertex) const
	{
		return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
			   (hash<glm::vec2>()(vertex.texPos) << 1);
	}
};
}

namespace dm
{


template<class VertexType = Vertex>
class Mesh : public IOwned<Device>
{
public:
	struct Data
	{
		std::vector<VertexType> vertices;
		std::vector<uint32_t> indices;
	};
	struct Memory
	{
		VertexType* vertices;
		uint32_t vertexCount;
		uint32_t* indices;
		uint32_t indexCount;
	};
	struct View
	{
		const VertexType* vertices;
		const uint32_t vertexCount;
		const uint32_t* indices;
		const uint32_t indexCount;
	};

	Mesh() = default;

	void Create(
		const std::vector<VertexType>& vertices,
		const std::vector<uint32_t>& indices,
		Device* inOwner,
		bool dynamic = false
	)
	{
        IOwned<Device>::CreateOwned(inOwner);
		vertexBuffer.Create(vertices, dynamic, owner);
		indexBuffer.Create(indices, dynamic, owner);
        sortKey = MeshSortKey<Mesh<VertexType>>::GetUnique();
	}

	void Create(
		const std::vector<VertexType>& vertices,
		Device* inOwner,
		bool dynamic = false
	)
	{
        IOwned<Device>::CreateOwned(inOwner);
		vertexBuffer.Create(vertices, dynamic, owner);
        sortKey = MeshSortKey<Mesh<VertexType>>::GetUnique();
    }

	Mesh& operator=(const Mesh& other) noexcept = delete;
	Mesh(const Mesh& other) noexcept = delete;

	Mesh(Mesh&& other) noexcept = default;
	Mesh& operator=(Mesh&& other) noexcept = default;
	~Mesh() noexcept override = default;

	void UpdateDynamic(
		std::vector<VertexType>& vertices,
		std::vector<uint32_t>& indices
	)
	{
		vertexBuffer.UpdateData(vertices.data(), vertices.size() * sizeof(VertexType), vertices.size(), false);
		indexBuffer.UpdateData(vertices.data(), vertices.size() * sizeof(uint32_t), indices.size(), false);
	}

	void UpdateDynamic(std::vector<VertexType>& vertices)
	{
		vertexBuffer.UpdateData(vertices.data(), vertices.size() * sizeof(VertexType), vertices.size(), false);
	}


	template<class T>
	std::vector<T> GetVertexBufferDataCopy(uint32_t offset) const
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

	[[nodiscard]] std::vector<VertexType> GetVertexBufferDataCopy() const
	{
		std::vector<VertexType> data;
		const auto* buffer = reinterpret_cast<const char*>(vertexBuffer.GetMappedData());
		uint32_t vertexCount = GetVertexCount();
		data.resize(vertexCount);
		memcpy(data.data(), buffer, sizeof(VertexType) * vertexCount);
		return std::move(data);
	}

	[[nodiscard]] std::vector<uint32_t> GetIndexBufferDataCopy() const
	{
		std::vector<uint32_t> data;
		const auto* buffer = reinterpret_cast<const char*>(indexBuffer.GetMappedData());
		data.resize(GetIndexCount());
		memcpy(data.data(), buffer, sizeof(uint32_t) * GetIndexCount());
		return std::move(data);
	}

	[[nodiscard]] Mesh::Data GetDataCopy() const
	{
		return {GetVertexBufferDataCopy(), GetIndexBufferDataCopy()};
	}

	VertexType* GetVertexBufferData()
	{
		return reinterpret_cast<VertexType*>(vertexBuffer.GetMappedData());
	}

	uint32_t* GetIndexBufferData()
	{
		return reinterpret_cast<uint32_t*>(indexBuffer.GetMappedData());
	}

	[[nodiscard]] const VertexType* GetVertexBufferData() const
	{
		return reinterpret_cast<const VertexType*>(vertexBuffer.GetMappedData());
	}

	[[nodiscard]] const uint32_t* GetIndexBufferData() const
	{
		return reinterpret_cast<const uint32_t*>(indexBuffer.GetMappedData());
	}

	[[nodiscard]] Mesh::View GetDataView() const
	{
		return {GetVertexBufferData(), GetVertexCount(),
				GetIndexBufferData(), GetIndexCount()
		};
	}


	void StageDynamic(vk::CommandBuffer commandBuffer)
	{
		vertexBuffer.StageTransferDynamic(commandBuffer);
		if (indexBuffer.GetIndexCount() > 0)
		{
			indexBuffer.StageTransferDynamic(commandBuffer);
		}
	}

	static Mesh::Data LoadModel(const std::string& path);

	void CreateModel(const std::string& path, bool dynamic, Device* owner)
	{
		DM_ASSERT_MSG(false, "There is no template specialization for this model creation.");
	}

	void Bind(vk::CommandBuffer commandBuffer) const
	{
		vk::DeviceSize offset = 0;
		commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer.VkType(), &offset);
		bool hasIndex = GetIndexCount() > 0;
		if (hasIndex)
		{
			commandBuffer.bindIndexBuffer(GetIndexBuffer().VkType(), 0, vk::IndexType::eUint32);
		}
	}

	void Draw(vk::CommandBuffer commandBuffer) const
	{
		bool hasIndex = GetIndexCount() > 0;
		hasIndex ?
		commandBuffer.drawIndexed(GetIndexCount(), 1, 0, 0, 0)
				 :
		commandBuffer.draw(GetVertexCount(), 1, 0, 0);
	}

	void SetModel(const glm::mat4& model)
	{
		this->model = model;
	}

	[[nodiscard]] uint32_t GetIndexCount() const
	{
		return indexBuffer.GetIndexCount();
	}

	[[nodiscard]] const IndexBuffer& GetIndexBuffer() const
	{
		return indexBuffer;
	}

	[[nodiscard]] uint32_t GetVertexCount() const
	{
		return vertexBuffer.GetVertexCount();
	}

	[[nodiscard]] std::weak_ptr<VertexBuffer < VertexType>> GetVertexBuffer() const
	{
		return vertexBuffer;
	}

    [[nodiscard]] MeshSortKey<VertexType> GetSortKey() const
    {
        return sortKey;
    }


    void DestroyVertexBuffer()
	{
		vertexBuffer.Destroy();
	}

	[[nodiscard]] std::vector<glm::vec3> AggregateVertexPositions() const
	{
		const uint32_t indexCount = GetIndexCount();
		const VertexType* vertices = GetVertexBufferData();
		std::vector<glm::vec3> positions;
		if (indexCount > 0)
		{
			const uint32_t* indices = GetIndexBufferData();
			positions.resize(indexCount);
			for (int i = 0; i < indexCount; ++i)
				positions[i] = vertices[indices[i]].pos;
		}
		else
		{
			const uint32_t vertexCount = GetVertexCount();
			positions.resize(vertexCount);
			for (int i = 0; i < vertexCount; ++i)
				positions[i] = vertices[i].pos;
		}

		return positions;
	}

	std::vector<glm::vec3> GetVertexEdgeListFromTriangleList() const
	{
		const uint32_t indexCount = GetIndexCount();
		const VertexType* vertices = GetVertexBufferData();
		std::vector<glm::vec3> positions;
		if (indexCount > 0)
		{
			const uint32_t* indices = GetIndexBufferData();
			for (int i = 0; i < indexCount; ++i)
			{
				positions.emplace_back(vertices[indices[i]].pos);
				if ((i + 1) % 3 == 0)
				{
					positions.emplace_back(vertices[indices[i - 2]].pos);
				}
				else
				{
					positions.emplace_back(vertices[indices[i + 1]].pos);
				}
			}
		}
		else
		{
			const uint32_t vertexCount = GetVertexCount();
			positions.resize(vertexCount * 4 / 3);
			for (int i = 0; i < vertexCount; ++i)
			{
				positions[i] = vertices[i].pos;
				if ((i + 1) % 3 == 0)
				{
					positions[++i] = vertices[i - 2].pos;
				}
			}
		}

		return positions;
	}


	glm::vec3 GetFurthestVertexPosition(const glm::vec3& direction) const;

	VertexBuffer <VertexType> vertexBuffer;
	IndexBuffer indexBuffer;
    MeshSortKey<Mesh<VertexType>> sortKey;
};

template<class VertexType>
glm::vec3 Mesh<VertexType>::GetFurthestVertexPosition(const glm::vec3& direction) const
{
	const auto* vertices = GetVertexBufferData();
	const auto vertexCount = GetVertexCount();

	int maxIndex = 0;
	float maxDot = -std::numeric_limits<float>::infinity();
	for (int i = 0; i < vertexCount; ++i)
	{
		float dotProduct = glm::dot(vertices[i].pos, direction);
		if (dotProduct > maxDot)
		{
			maxIndex = i;
			maxDot = dotProduct;
		}
	}

	return vertices[maxIndex].pos;
}


void InitializeMeshStatics(Device* device);
void DestroyMeshStatics();


template<class VertexType>
struct SimpleMesh
{
    static void Initialize(Device* owner);
    static void Destroy();
    inline static Mesh<VertexType>* Sphere = nullptr;
    inline static Mesh<VertexType>* Cube = nullptr;
    inline static Mesh<VertexType>* Point = nullptr;
    inline static Mesh<VertexType>* Plane = nullptr;
    inline static Mesh<VertexType>* Triangle = nullptr;
    inline static Mesh<VertexType>* Quad = nullptr;
    inline static Mesh<VertexType>* Ray = nullptr;

    inline static Mesh<VertexType>* SquareLineStrip = nullptr;
    inline static Mesh<VertexType>* CircleLineStrip = nullptr;

    // Created using line lists instead of strips
    inline static Mesh<VertexType>* CubeList = nullptr;
};

template<class VertexType>
inline typename Mesh<VertexType>::Data Mesh<VertexType>::LoadModel(const std::string& path)
{
	DM_ASSERT_MSG(false, "No template specialization for loading a model with this vertex type");
}

template<>
inline typename Mesh<Vertex>::Data Mesh<Vertex>::LoadModel(const std::string& path)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, error;

	DM_ASSERT_MSG(
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

			if (uniqueVerts.count(vertex) == 0)
			{
				uniqueVerts[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVerts[vertex]);
		}
	}

	return {std::move(vertices), std::move(indices)};
}
}

