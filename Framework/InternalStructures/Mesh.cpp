//------------------------------------------------------------------------------
//
// File Name:	Mesh.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/8/2020
//
//------------------------------------------------------------------------------
namespace bk {
//template<>
//Mesh<Vertex> Mesh<Vertex>::Sphere;
//template<>
//Mesh<Vertex> Mesh<Vertex>::Cube;
//template<>
//Mesh<Vertex> Mesh<Vertex>::Point;
//template<>
//Mesh<Vertex> Mesh<Vertex>::Triangle;
//
//template<>
//Mesh<PosVertex> Mesh<PosVertex>::Sphere;
//template<>
//Mesh<PosVertex> Mesh<PosVertex>::Cube;
//template<>
//Mesh<PosVertex> Mesh<PosVertex>::Point;
//template<>
//Mesh<PosVertex> Mesh<PosVertex>::Plane;
//template<>
//Mesh<PosVertex> Mesh<PosVertex>::Triangle;
//template<>
//Mesh<PosVertex> Mesh<PosVertex>::Ray;
//
//template<>
//Mesh<PosVertex> Mesh<PosVertex>::CubeList;

const std::vector<PosVertex> planeVerts = {
	{{-1.0, 0.0, -1.0}},    // 0
	{{-1.0, 0.0, 1.0}},        // 1
	{{1.0,  0.0, 1.0}},    // 2
	{{1.0,  0.0, -1.0}}   // 3
};

const std::vector<uint32_t> planeIndices = {
	0, 1, 2,
	2, 3, 0
};

const std::vector<PosVertex> rayVerts = {
	{{0.0, 0.0, 0.0}},    // 0
	{{0.0, 0.0, 1.0}}   // 1
};

const std::vector<PosVertex> triVerts = {
	{{0.0,  1.0,  0.0}},    // 0
	{{1.0,  -1.0, 0.0}},  // 1
	{{-1.0, -1.0, 0.0}}   // 2
};


void InitializeMeshStatics(Device * inOwner)
{
	SimpleMesh<Vertex>::Cube = new Mesh<Vertex>();
	SimpleMesh<Vertex>::CubeList = new Mesh<Vertex>();
	SimpleMesh<Vertex>::Point = new Mesh<Vertex>();
	SimpleMesh<Vertex>::Plane = new Mesh<Vertex>();
	SimpleMesh<Vertex>::Triangle = new Mesh<Vertex>();
	SimpleMesh<Vertex>::Sphere = new Mesh<Vertex>();
	SimpleMesh<Vertex>::Ray = new Mesh<Vertex>();

	SimpleMesh<PosVertex>::Cube = new Mesh<PosVertex>();
	SimpleMesh<PosVertex>::CubeList = new Mesh<PosVertex>();
	SimpleMesh<PosVertex>::Point = new Mesh<PosVertex>();
	SimpleMesh<PosVertex>::Plane = new Mesh<PosVertex>();
	SimpleMesh<PosVertex>::Triangle = new Mesh<PosVertex>();
	SimpleMesh<PosVertex>::Sphere = new Mesh<PosVertex>();
	SimpleMesh<PosVertex>::Ray = new Mesh<PosVertex>();

	std::vector<Vertex> vertices;

	int i;
#define Band_Power  8  // 2^Band_Power = Total Points in a band.
#define Band_Points 256 // 16 = 2^Band_Power
#define Band_Mask (Band_Points-2)
#define Sections_In_Band ((Band_Points/2)-1)
#define Total_Points (Sections_In_Band*Band_Points)
	// remember - for each section in a band, we have a band
#define Section_Arc (6.28/Sections_In_Band)
	const float R = -0.5; // radius of 10

	float x_angle;
	float y_angle;

	for (i = 0; i < Total_Points; i++)
	{
		// using last bit to alternate,+band number (which band)
		x_angle = (float) (i & 1) + (i >> Band_Power);

		// (i&Band_Mask)>>1 == Local Y value in the band
		// (i>>Band_Power)*((Band_Points/2)-1) == how many bands
		//  have we processed?
		// Remember - we go "right" one value for every 2 points.
		//  i>>bandpower - tells us our band number
		y_angle = (float) ((i & Band_Mask) >> 1) + ((i >> Band_Power) * (Sections_In_Band));

		x_angle *= (float) Section_Arc / 2.0f; // remember - 180� x rot not 360
		y_angle *= (float) Section_Arc * -1;


		Vertex vertex;
		vertex.pos = {
			R * sin(x_angle) * sin(y_angle),
			R * cos(x_angle),
			R * sin(x_angle) * cos(y_angle)
		};
		vertices.emplace_back(vertex);
	}
	*SimpleMesh<Vertex>::Sphere = Mesh<Vertex>(vertices, inOwner);


	std::vector<Vertex> cubeVerts = {
		{{-0.5f, -0.5f, 0.5f},  {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}},
		{{0.5f,  -0.5f, 0.5f},  {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},

		{{0.5f,  0.5f,  0.5f},  {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}},
		{{-0.5f, 0.5f,  0.5f},  {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}},

		{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
		{{0.5f,  -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},

		{{0.5f,  0.5f,  -0.5f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f,  -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
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


	*SimpleMesh<Vertex>::Cube = Mesh<Vertex>(cubeVerts, cubeIndices, inOwner);

	std::vector<Vertex> pt = {
		{{0.0, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}}
	};
	*SimpleMesh<Vertex>::Point = Mesh<Vertex>(pt, inOwner);

	// POS VERTEX
	auto cubePositions = SimpleMesh<Vertex>::Cube->GetVertexBufferDataCopy<glm::vec3>(0);
	std::vector<PosVertex> cubePosVertices;
	cubePosVertices.resize(cubePositions.size());
	std::memcpy(cubePosVertices.data(), cubePositions.data(), sizeof(glm::vec3) * cubePositions.size());

	auto spherePositions2 = SimpleMesh<Vertex>::Sphere->GetVertexBufferDataCopy<glm::vec3>(0);
	std::vector<PosVertex> spherePosVertices;
	spherePosVertices.resize(spherePositions2.size());
	std::memcpy(spherePosVertices.data(), spherePositions2.data(), sizeof(glm::vec3) * spherePositions2.size());


	std::vector<PosVertex> point = {
		{{0.0f, 0.0f, 0.0f}}
	};

	*SimpleMesh<PosVertex>::Cube = Mesh<PosVertex>(cubePosVertices, cubeIndices, inOwner);
	*SimpleMesh<PosVertex>::Sphere = Mesh<PosVertex>(spherePosVertices, inOwner);
	*SimpleMesh<PosVertex>::Plane = Mesh<PosVertex>(planeVerts, planeIndices, inOwner);
	*SimpleMesh<PosVertex>::Point = Mesh<PosVertex>(point, inOwner);
	*SimpleMesh<PosVertex>::Ray = Mesh<PosVertex>(rayVerts, inOwner);
	*SimpleMesh<PosVertex>::Triangle = Mesh<PosVertex>(triVerts, inOwner);

	const std::vector<uint32_t> cubeListIndices =
		{
			// Bottom square
			// - - - to - - +
			4, 0,
			// - - + to + - +
			0, 1,
			// + - + to + - -
			1, 5,
			// + - - to - - -
			5, 4,

			// Top square
			// - + - to - + +
			7, 3,
			// - + + to + + +
			3, 2,
			// + + + to + + -
			2, 6,
			// + + - to - + -
			6, 7,

			// Lines connecting the two
			// - - - to - + -
			4, 7,
			// - - + to - + +
			0, 3,
			// + - + to + + +
			1, 2,
			// + - - to + + -
			5, 6
		};

	*SimpleMesh<PosVertex>::CubeList = Mesh<PosVertex>(cubePosVertices, cubeListIndices, inOwner);
	std::vector<Vertex> triangleVertices;
	triangleVertices.resize(triVerts.size());
	for (int i = 0; i < triVerts.size(); ++i)
	{
		triangleVertices[i].pos = triVerts[i].pos;
	}
	std::vector<uint32_t> triIndices = {0, 1, 2};
	*SimpleMesh<Vertex>::Triangle = Mesh<Vertex>(triangleVertices, triIndices, inOwner);
}

void DestroyMeshStatics()
{
	delete SimpleMesh<Vertex>::Cube ;
	delete SimpleMesh<Vertex>::CubeList ;
	delete SimpleMesh<Vertex>::Point ;
	delete SimpleMesh<Vertex>::Plane ;
	delete SimpleMesh<Vertex>::Triangle ;
	delete SimpleMesh<Vertex>::Sphere ;
	delete SimpleMesh<Vertex>::Ray ;

	delete SimpleMesh<PosVertex>::Cube ;
	delete SimpleMesh<PosVertex>::CubeList ;
	delete SimpleMesh<PosVertex>::Point ;
	delete SimpleMesh<PosVertex>::Plane ;
	delete SimpleMesh<PosVertex>::Triangle ;
	delete SimpleMesh<PosVertex>::Sphere ;
	delete SimpleMesh<PosVertex>::Ray ;
}

}