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

namespace dm {


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
		IOwned<Device>::Create(inOwner);
		vertexBuffer.Create(vertices, dynamic, owner);
		indexBuffer.Create(indices, dynamic, owner);
	}

	void Create(
		const std::vector<VertexType>& vertices,
		Device* inOwner,
		bool dynamic = false
	)
	{
		IOwned<Device>::Create(inOwner);
		vertexBuffer.Create(vertices, dynamic, owner);
	}

	Mesh& operator=(const Mesh& other) noexcept = delete;
	Mesh(const Mesh& other) noexcept = delete;

	Mesh(Mesh&& other) noexcept = default;
	Mesh& operator=(Mesh&& other) noexcept = default;
	~Mesh() noexcept = default;

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
		DM_ASSERT(false, "There is no template specialization for this model creation.");
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


	// -1 if all in the back, 0 if straddling, and 1 if all in front
	//int IsStraddlingPlane(const Primitives::Plane& plane) const;
	static int IsStraddlingPlane(
		const VertexType* vertices,
		const uint32_t vertexCount,
		const Primitives::Plane& plane,
		float* minDistance = nullptr
	);


	// Returns front-facing and back-facing mesh relative to the plane respectively
	[[nodiscard]] std::pair<Mesh<VertexType>, Mesh<VertexType>> Clip(const Primitives::Plane& plane) const;

	static void Clip(
		const Mesh<VertexType>::Data& data,
		const Primitives::Plane& plane,
		Mesh<VertexType>::Data& front,
		Mesh<VertexType>::Data& back
	);

	static Primitives::Box GetBoundingBox(const VertexType* vertices, const uint32_t vertexCount);

	Primitives::Box GetBoundingBox() const;

	glm::vec3 GetFurthestVertexPosition(const glm::vec3& direction) const;

	VertexBuffer <VertexType> vertexBuffer;
	IndexBuffer indexBuffer;
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

template<class VertexType>
Primitives::Box Mesh<VertexType>::GetBoundingBox(const VertexType* vertices, const uint32_t vertexCount)
{
	glm::vec3 minX, minY, minZ;
	glm::vec3 maxX, maxY, maxZ;

	auto GetExtremePoints = [vertices, vertexCount](auto& min, auto& max, const auto& dir)
	{
		float minProj = FLT_MAX;
		float maxProj = -FLT_MAX;
		int indexMin = 0, indexMax = 0;

		for (int i = 0; i < vertexCount; ++i)
		{
			float proj = glm::dot(vertices[i].pos, dir);

			if (proj < minProj)
			{
				minProj = proj;
				indexMin = i;
			}
			if (proj > maxProj)
			{
				maxProj = proj;
				indexMax = i;
			}
		}
		min = vertices[indexMin].pos;
		max = vertices[indexMax].pos;
	};

	GetExtremePoints(minX, maxX, glm::vec3(1.0f, 0.0f, 0.0f));
	GetExtremePoints(minY, maxY, glm::vec3(0.0f, 1.0f, 0.0f));
	GetExtremePoints(minZ, maxZ, glm::vec3(0.0f, 0.0f, 1.0f));


	Primitives::Box bb;
	bb.position = glm::vec3(maxX.x + minX.x, maxY.y + minY.y, maxZ.z + minZ.z) * 0.5f;
	bb.halfExtent = glm::vec3(maxX.x - minX.x, maxY.y - minY.y, maxZ.z - minZ.z) * 0.5f;

	return bb;
}

template<class VertexType>
Primitives::Box Mesh<VertexType>::GetBoundingBox() const
{
	return GetBoundingBox(GetVertexBufferData(), GetVertexCount());
}


template<class VertexType>
int Mesh<VertexType>::IsStraddlingPlane(
	const VertexType* vertices,
	const uint32_t vertexCount,
	const Primitives::Plane& plane,
	float* minDistance
)
{
	uint32_t positive = 0;
	uint32_t negative = 0;

	float minDist = std::numeric_limits<float>::max();

	//DM_ASSERT(planeDistance.size() == size, "Incorrect size for input bool vector");
	for (int i = 0; i < vertexCount; ++i)
	{
		const glm::vec3& vert = vertices[i].pos;
		float distance = glm::dot(plane.normal, vert - plane.position);
		if (distance > Primitives::Plane::thickness)
		{
			++positive;
		}
		else if (distance < -Primitives::Plane::thickness)
		{
			++negative;
		}


		if (minDistance != nullptr)
		{
			float absDist = std::abs(distance);
			if (absDist < minDist)
			{
				minDist = absDist;
			}
		}
	}

	if (minDistance != nullptr)
	{
		*minDistance = minDist;
	}

	if (!negative && !positive)
	{
		return 1;
	}

	int straddle = 0;
	straddle += negative == 0;
	straddle -= positive == 0;

	return straddle;
}


//template <class VertexType>
//int Mesh<VertexType>::IsStraddlingPlane(const Primitives::Plane& plane) const
//{
//	uint32_t positive = 0;
//	uint32_t negative = 0;
//
//	auto vertCount = GetVertexCount();
//	const VertexType* vertices = GetVertexBufferData();
//	for (int i = 0; i < vertCount; ++i)
//	{
//		const VertexType& vert = vertices[i];
//		float distance = glm::dot(plane.normal, vert.pos - plane.position);
//		if (distance > Primitives::Plane::thickness)
//			++positive;
//		else if (distance < -Primitives::Plane::thickness)
//			++negative;
//	}
//
//	int straddle = 0;
//	straddle += negative == 0;
//	straddle -= positive == 0;
//
//	return straddle;
//}

void InitializeMeshStatics(Device* device);
void DestroyMeshStatics();


template<class VertexType>
void Mesh<VertexType>::Clip(
	const Mesh<VertexType>::Data& data,
	const Primitives::Plane& plane,
	Mesh<VertexType>::Data& front,
	Mesh<VertexType>::Data& back
)
{
	int frontCount = 0, backCount = 0;

	int size = data.indices.size();
	DM_ASSERT(size % 3 == 0, "Incorrect number of vertices attempted to be split");
	DM_ASSERT(data.indices.size() != 0, "Non-indexed triangles not supported for octree");
	std::vector<glm::vec3> frontVerts, backVerts;
	frontVerts.resize(size * 4 / 3);
	backVerts.resize(size * 4 / 3);

	// Used to store per triangle if it is a quad
	std::vector<bool> frontQuads(size / 3);
	std::vector<bool> backQuads(size / 3);

	int currentFrontFaceVertCount = 0;
	int currentBackFaceVertCount = 0;
	int frontFaceCount = 0;
	int backFaceCount = 0;

	bool triCross = false;

	for (int i = 0; i < size; i += 3)
	{
		uint32_t indices[3] = {
			data.indices[i + 0],
			data.indices[i + 1],
			data.indices[i + 2],
		};

		PosVertex tri[3] = {
			data.vertices[indices[0]],
			data.vertices[indices[1]],
			data.vertices[indices[2]]
		};

		int straddle = IsStraddlingPlane(tri, 3u, plane);
		if (straddle != 0)
		{
			if (straddle == 1)
			{
				for (int j = 0; j < 3; ++j)
					frontVerts[frontCount++] = tri[j].pos;
				// All on one side, not a quad
				frontQuads[frontFaceCount++] = false;
			}
			else
			{
				for (int j = 0; j < 3; ++j)
					backVerts[backCount++] = tri[j].pos;
				// All on one side, not a quad
				backQuads[backFaceCount++] = false;
			}
			continue;
		}

		auto SplitTriangle =
			[&](const glm::vec3& a, const glm::vec3& b)
			{
				int aSide = ClassifyPointToPlane(a, plane);
				int bSide = ClassifyPointToPlane(b, plane);
				if (bSide == Primitives::PointPlaneStatus::Front)
				{
					if (aSide == Primitives::PointPlaneStatus::Back)
					{
						Primitives::Ray ray;
						ray.direction = glm::normalize(b - a);
						ray.position = a;
						glm::vec3 i = Primitives::GetRayPlaneIntersection(ray, plane);
						//DM_ASSERT(ClassifyPointToPlane(i, plane) == Primitives::PointPlaneStatus::Coplanar , "Failed validation of clipping");
						frontVerts[frontCount++] = backVerts[backCount++] = i;
						++currentFrontFaceVertCount;
						++currentBackFaceVertCount;
					}
					// In all three cases, output b to the front side
					frontVerts[frontCount++] = b;
					++currentFrontFaceVertCount;
				}
				else if (bSide == Primitives::PointPlaneStatus::Back)
				{
					if (aSide == Primitives::PointPlaneStatus::Front)
					{
						//float t = da / (da - db);
						//glm::vec3 i = (1 - t) * a + t * b;
						Primitives::Ray ray;
						ray.direction = glm::normalize(b - a);
						ray.position = a;
						glm::vec3 i = Primitives::GetRayPlaneIntersection(ray, plane);

						// Edge (a, b) straddles plane, output intersection point
						//DM_ASSERT(ClassifyPointToPlane(i, plane) == Primitives::PointPlaneStatus::Coplanar, "Failed validation of clipping");
						frontVerts[frontCount++] = backVerts[backCount++] = i;
						++currentFrontFaceVertCount;
						++currentBackFaceVertCount;
					}
					else if (aSide == Primitives::PointPlaneStatus::Coplanar)
					{
						// Output a when edge (a, b) goes from �on� to �behind� plane
						backVerts[backCount++] = a;
						++currentBackFaceVertCount;
					}
					// In all three cases, output b to the back side
					backVerts[backCount++] = b;
					++currentBackFaceVertCount;
				}
				else
				{
					// b is on the plane. In all three cases output b to the front side
					frontVerts[frontCount++] = b;
					++currentFrontFaceVertCount;
					// In one case, also output b to back side
					if (aSide == Primitives::PointPlaneStatus::Back)
					{
						backVerts[backCount++] = b;
						++currentBackFaceVertCount;
					}
				}
			};

		SplitTriangle(tri[0].pos, tri[1].pos);
		SplitTriangle(tri[1].pos, tri[2].pos);
		SplitTriangle(tri[2].pos, tri[0].pos);

		// We have finished this triangle set, output 
		// whether or not it is a tri or quad
		DM_ASSERT(currentFrontFaceVertCount == 3 ||
			   currentFrontFaceVertCount == 4 ||
			   currentFrontFaceVertCount == 0, "Invalid number of face points when clipping!");
		DM_ASSERT(currentBackFaceVertCount == 3 ||
			   currentBackFaceVertCount == 4 ||
			   currentBackFaceVertCount == 0, "Invalid number of face points when clipping!");
		if (currentFrontFaceVertCount > 2)
		{
			frontQuads[frontFaceCount++] = (currentFrontFaceVertCount == 4);
		}
		if (currentBackFaceVertCount > 2)
		{
			backQuads[backFaceCount++] = (currentBackFaceVertCount == 4);
		}

		currentFrontFaceVertCount = 0;
		currentBackFaceVertCount = 0;
	}
	frontQuads.resize(frontFaceCount);
	backQuads.resize(backFaceCount);

	auto SplitQuads =
		[](
			const std::vector<bool>& quads,
			const std::vector<glm::vec3>& input,
			const int faceVertCount,
			Mesh<VertexType>::Data& out
		)
		{
			int numFaces = quads.size();

			int numOutputVertices = 0;
			int numOutputIndices = 0;
			for (int i = 0; i < numFaces; ++i)
			{
				numOutputVertices += (quads[i] == true) ? 4 : 3;
				numOutputIndices += (quads[i] == true) ? 6 : 3;
			}

			out.vertices.resize(numOutputVertices);
			out.indices.resize(numOutputIndices);

			for (int i = 0, faceIndex = 0, ii = 0; i < faceVertCount;)
			{
				bool isQuad = (quads[faceIndex++] == true);

				for (int j = 0; j < 3; ++j)
				{
					out.vertices[i + j].pos = input[i + j];
					out.indices[ii + j] = i + j;
				}

				if (isQuad)
				{
					out.vertices[i + 3].pos = input[i + 3];
					// 0 2 3
					out.indices[ii + 3] = i + 0;
					out.indices[ii + 4] = i + 2;
					out.indices[ii + 5] = i + 3;
					i += 4;
					ii += 6;
				}
				else
				{
					i += 3;
					ii += 3;
				}
			}
		};
	if (frontCount > 0)
	{
		SplitQuads(frontQuads, frontVerts, frontCount, front);
	}
	if (backCount > 0)
	{
		SplitQuads(backQuads, backVerts, backCount, back);
	}

	DM_ASSERT(front.indices.size() % 3 == 0, "Incorrect number of indices in front output triangles");
	DM_ASSERT(back.indices.size() % 3 == 0, "Incorrect number of indices in back output triangles");
}

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
	inline static Mesh<VertexType>* Ray = nullptr;

	// Created using line lists instead of strips
	inline static Mesh<VertexType>* CubeList = nullptr;
};


//template <class VertexType>
//std::pair<Mesh<VertexType>, Mesh<VertexType>> Mesh<VertexType>::Clip(const Primitives::Plane& plane) const
//{
//	int indexCount = GetIndexCount();
//	const bool useIndices = (indexCount != 0);
//	const uint32_t* indices = (useIndices) ? GetIndexBufferData() : nullptr;
//	int vertexCount = GetVertexCount();
//	vertices = GetVertexBufferData();
//
//}


template<class VertexType>
inline typename Mesh<VertexType>::Data Mesh<VertexType>::LoadModel(const std::string& path)
{
	DM_ASSERT(false, "No template specialization for loading a model with this vertex type");
}


template<>
inline typename Mesh<Vertex>::Data Mesh<Vertex>::LoadModel(const std::string& path)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, error;

	DM_ASSERT(
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

//template<>
//inline void Mesh<Vertex>::CreateModel(const std::string& path, bool dynamic, Device* owner)
//{
//	auto data = LoadModel(path);
//	if (dynamic)
//	{
//		CreateDynamic(data.vertices, data.indices, owner);
//	}
//	else
//	{
//		CreateStatic(data.vertices, data.indices, owner);
//	}
}

