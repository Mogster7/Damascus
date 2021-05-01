//------------------------------------------------------------------------------
//
// File Name:	Mesh.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/8/2020
//
//------------------------------------------------------------------------------
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
	 { {-1.0, 0.0, -1.0 } },	// 0
	 { { -1.0, 0.0, 1.0 } },	    // 1
	 { { 1.0, 0.0, 1.0 } },    // 2
	 { { 1.0, 0.0, -1.0 } }   // 3
};

const std::vector<uint32_t> planeIndices = {
	0, 1, 2,
	2, 3, 0
};

const std::vector<PosVertex> rayVerts = {
	 { { 0.0, 0.0, 0.0 } },	// 0
	 { { 0.0, 0.0, 1.0 } }   // 1
};

const std::vector<PosVertex> triVerts = {
	 { { 0.0, 1.0, 0.0 } },	// 0
	 { { 1.0, -1.0, 0.0 } },  // 1
	 { { -1.0, -1.0, 0.0 } }   // 2
};


void InitializeMeshStatics(Device& device)
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
	const float R = -0.5; // radius of 10

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
	Mesh<Vertex>::Sphere.CreateStatic(vertices, device);


	std::vector<Vertex> cubeVerts = {
	{ { -0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f, 1.0f } , { 1.0f, 0.0f, 1.0f }},
	{ { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } , { 0.0f, 0.0f, 1.0f }},

	{ { 0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 1.0f } , { 0.0f, 1.0f, 1.0f } },
	{ { -0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 1.0f } , { 1.0f, 0.0f, 1.0f }},

	{ { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f  }, { 0.0f, 0.0f, 0.0f}},
	{ { 0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f  }},

	{ { 0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f } , { 1.0f, 1.0f, 0.0f }},
	{ { -0.5f, 0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f  }},
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

	Mesh<Vertex>::Cube.CreateStatic(cubeVerts, cubeIndices, device);

	std::vector<Vertex> pt = {
	{ { 0.0, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } , { 1.0f, 0.0f, 1.0f }}
	};
	Mesh<Vertex>::Point.CreateStatic(pt, device);

	// POS VERTEX
	auto cubePositions = Mesh<Vertex>::Cube.GetVertexBufferDataCopy<glm::vec3>(0);
	std::vector<PosVertex> cubePosVertices;
	cubePosVertices.resize(cubePositions.size());
	std::memcpy(cubePosVertices.data(), cubePositions.data(), sizeof(glm::vec3) * cubePositions.size());

	auto spherePositions2 = Mesh<Vertex>::Sphere.GetVertexBufferDataCopy<glm::vec3>(0);
	std::vector<PosVertex> spherePosVertices;
	spherePosVertices.resize(spherePositions2.size());
	std::memcpy(spherePosVertices.data(), spherePositions2.data(), sizeof(glm::vec3) * spherePositions2.size());


	std::vector<PosVertex> point = { 
		{ { 0.0f, 0.0f, 0.0f } }
	};

	Mesh<PosVertex>::Cube.CreateStatic(cubePosVertices, cubeIndices, device);
	Mesh<PosVertex>::Sphere.CreateStatic(spherePosVertices, device);
	Mesh<PosVertex>::Plane.CreateStatic(planeVerts, planeIndices, device);
	Mesh<PosVertex>::Point.CreateStatic(point, device);
	Mesh<PosVertex>::Ray.CreateStatic(rayVerts, device);
	Mesh<PosVertex>::Triangle.CreateStatic(triVerts, device);

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

	Mesh<PosVertex>::CubeList.CreateStatic(cubePosVertices, cubeListIndices, device);
	std::vector<Vertex> triangleVertices;
	triangleVertices.resize(triVerts.size());
	for (int i = 0; i < triVerts.size(); ++i)
	{
		triangleVertices[i].pos = triVerts[i].pos;
	}
	std::vector<uint32_t> triIndices = { 0, 1, 2 };
	Mesh<Vertex>::Triangle.CreateStatic(triangleVertices, triIndices, device);
}
