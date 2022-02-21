//------------------------------------------------------------------------------
//
// File Name:	Mesh.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/8/2020
//
//------------------------------------------------------------------------------
namespace dm
{
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
    { { -0.5, 0.0, -0.5 } }, // 0
    { { -0.5, 0.0, 0.5 } },  // 1
    { { 0.5, 0.0, 0.5 } },   // 2
    { { 0.5, 0.0, -0.5 } }   // 3
};

const std::vector<uint32_t> planeIndices = {
    0, 1, 2,
    2, 3, 0
};

const std::vector<PosVertex> rayVerts = {
    { { 0.0, -0.5, 0.0 } }, // 0
    { { 0.0, 0.5, 0.0 } }   // 1
};

const std::vector<PosVertex> triVerts = {
    { { 0.0, 0.5, 0.0 } },  // 0
    { { 0.5, -0.5, 0.0 } }, // 1
    { { -0.5, -0.5, 0.0 } } // 2
};

const std::vector<PosVertex> quadPosVerts = {
    { { -0.5, 0.5, 0.0 } }, // 0
    { { 0.5, 0.5, 0.0 } },  // 1
    { { 0.5, -0.5, 0.0 } }, // 2
    { { -0.5, -0.5, 0.0 } } // 3
};

const std::vector<PosVertex> squareStripVerts = {
    { { -0.5f, -0.5f, 0.0f } },
    { { -0.5f, 0.5f, 0.0f } },
    { { 0.5f, 0.5f, 0.0f } },
    { { 0.5f, -0.5f, 0.0f } },
    { { -0.5f, -0.5f, 0.0f } }
};


void InitializeMeshStatics(Device* inOwner)
{
    SimpleMesh<Vertex>::Cube = new Mesh<Vertex>();
    SimpleMesh<Vertex>::CubeList = new Mesh<Vertex>();
    SimpleMesh<Vertex>::Point = new Mesh<Vertex>();
    SimpleMesh<Vertex>::Plane = new Mesh<Vertex>();
    SimpleMesh<Vertex>::Triangle = new Mesh<Vertex>();
    SimpleMesh<Vertex>::Quad = new Mesh<Vertex>();
    SimpleMesh<Vertex>::Sphere = new Mesh<Vertex>();
    SimpleMesh<Vertex>::Ray = new Mesh<Vertex>();
    SimpleMesh<Vertex>::SquareLineStrip = new Mesh<Vertex>();
    SimpleMesh<Vertex>::CircleLineStrip = new Mesh<Vertex>();

    SimpleMesh<PosVertex>::Cube = new Mesh<PosVertex>();
    SimpleMesh<PosVertex>::CubeList = new Mesh<PosVertex>();
    SimpleMesh<PosVertex>::Point = new Mesh<PosVertex>();
    SimpleMesh<PosVertex>::Plane = new Mesh<PosVertex>();
    SimpleMesh<PosVertex>::Triangle = new Mesh<PosVertex>();
    SimpleMesh<PosVertex>::Quad = new Mesh<PosVertex>();
    SimpleMesh<PosVertex>::Sphere = new Mesh<PosVertex>();
    SimpleMesh<PosVertex>::Ray = new Mesh<PosVertex>();
    SimpleMesh<PosVertex>::SquareLineStrip = new Mesh<PosVertex>();
    SimpleMesh<PosVertex>::CircleLineStrip = new Mesh<PosVertex>();

    SimpleMesh<TexVertex>::Quad = new Mesh<TexVertex>();

    std::vector<Vertex> vertices;

    int i;
#define Band_Power 8    // 2^Band_Power = Total Points in a band.
#define Band_Points 256 // 16 = 2^Band_Power
#define Band_Mask (Band_Points - 2)
#define Sections_In_Band ((Band_Points / 2) - 1)
#define Total_Points (Sections_In_Band * Band_Points)
    // remember - for each section in a band, we have a band
#define Section_Arc (6.28 / Sections_In_Band)
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
    SimpleMesh<Vertex>::Sphere->Create(vertices, inOwner);


    std::vector<Vertex> cubeVerts = {
        { { -0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 1.0f } },
        { { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },

        { { 0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 1.0f }, { 0.0f, 1.0f, 1.0f } },
        { { -0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 1.0f } },

        { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
        { { 0.5f, -0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f } },

        { { 0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f } },
        { { -0.5f, 0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
    };

    const std::vector<uint32_t> cubeIndices = {
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


    SimpleMesh<Vertex>::Cube->Create(cubeVerts, cubeIndices, inOwner);

    std::vector<Vertex> pt = {
        { { 0.0, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 1.0f } }
    };
    SimpleMesh<Vertex>::Point->Create(pt, inOwner);

	// POS VERTEX
/*	auto cubePositions = SimpleMesh<Vertex>::Cube->GetVertexBufferDataCopy<glm::vec3>(0);
	std::vector<PosVertex> cubePosVertices;
	cubePosVertices.resize(cubePositions.size());
	std::memcpy(cubePosVertices.data(), cubePositions.data(), sizeof(glm::vec3) * cubePositions.size());

	auto spherePositions2 = SimpleMesh<Vertex>::Sphere->GetVertexBufferDataCopy<glm::vec3>(0);
	std::vector<PosVertex> spherePosVertices;
	spherePosVertices.resize(spherePositions2.size());
	std::memcpy(spherePosVertices.data(), spherePositions2.data(), sizeof(glm::vec3) * spherePositions2.size());*/

    std::vector<PosVertex> point = {
        { { 0.0f, 0.0f, 0.0f } }
    };

/*	SimpleMesh<PosVertex>::Cube->Create(cubePosVertices, cubeIndices, inOwner);
	SimpleMesh<PosVertex>::Sphere->Create(spherePosVertices, inOwner);*/
	SimpleMesh<PosVertex>::Plane->Create(planeVerts, planeIndices, inOwner);
	SimpleMesh<PosVertex>::Point->Create(point, inOwner);
	SimpleMesh<PosVertex>::Ray->Create(rayVerts, inOwner);
	SimpleMesh<PosVertex>::Triangle->Create(triVerts, inOwner);

    const std::vector<uint32_t> cubeListIndices = {
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

    //SimpleMesh<PosVertex>::CubeList->Create(cubePosVertices, cubeListIndices, inOwner);

    // Triangle.
    std::vector<Vertex> triangleVertices;
    triangleVertices.resize(triVerts.size());
    for (int i = 0; i < triVerts.size(); ++i)
    {
        triangleVertices[i].pos = triVerts[i].pos;
    }
    std::vector<uint32_t> triIndices = { 0, 1, 2 };
    SimpleMesh<Vertex>::Triangle->Create(triangleVertices, triIndices, inOwner);

    // Quad.
    std::vector<Vertex> quadVertices;
    std::vector<TexVertex> texQuadVertices;
    quadVertices.resize(quadPosVerts.size());
    texQuadVertices.resize(quadPosVerts.size());
    for (int i = 0; i < quadPosVerts.size(); ++i)
    {
        quadVertices[i].pos = quadPosVerts[i].pos;
        texQuadVertices[i].pos = quadVertices[i].pos * 2.0f;
        quadVertices[i].texPos = glm::vec2(quadPosVerts[i].pos.x, -quadPosVerts[i].pos.y) + glm::vec2(0.5f);
        texQuadVertices[i].texPos = glm::vec2(quadPosVerts[i].pos.x, quadPosVerts[i].pos.y) + glm::vec2(0.5f);
    }
    std::vector<uint32_t> quadIndices = { 0, 1, 2, 0, 2, 3 };
    SimpleMesh<Vertex>::Quad->Create(quadVertices, quadIndices, inOwner);
    SimpleMesh<PosVertex>::Quad->Create(quadPosVerts, quadIndices, inOwner);
    SimpleMesh<TexVertex>::Quad->Create(texQuadVertices, quadIndices, inOwner);

    SimpleMesh<PosVertex>::SquareLineStrip->Create(squareStripVerts, inOwner);

    // Circle.
    // Generate vertices.
    int numVertices = 30;
    std::vector<PosVertex> circleVertices{};
    for (int i = 0; i < numVertices; ++i)
    {
        float theta = 2.0f * static_cast<float>(M_PI) * i / (float) numVertices;
        circleVertices.emplace_back(PosVertex{ { glm::cos(theta), glm::sin(theta), 0.0f } });
    }
    circleVertices.emplace_back(PosVertex{ { 1.0f, 0.0f, 0.0f } }); // Last vertex.

    SimpleMesh<PosVertex>::CircleLineStrip->Create(circleVertices, inOwner);
}

void DestroyMeshStatics()
{
    delete SimpleMesh<Vertex>::Cube;
    delete SimpleMesh<Vertex>::CubeList;
    delete SimpleMesh<Vertex>::Point;
    delete SimpleMesh<Vertex>::Plane;
    delete SimpleMesh<Vertex>::Triangle;
    delete SimpleMesh<Vertex>::Quad;
    delete SimpleMesh<Vertex>::Sphere;
    delete SimpleMesh<Vertex>::Ray;
    delete SimpleMesh<Vertex>::SquareLineStrip;
    delete SimpleMesh<Vertex>::CircleLineStrip;

    delete SimpleMesh<PosVertex>::Cube;
    delete SimpleMesh<PosVertex>::CubeList;
    delete SimpleMesh<PosVertex>::Point;
    delete SimpleMesh<PosVertex>::Plane;
    delete SimpleMesh<PosVertex>::Triangle;
    delete SimpleMesh<PosVertex>::Quad;
    delete SimpleMesh<PosVertex>::Sphere;
    delete SimpleMesh<PosVertex>::Ray;
    delete SimpleMesh<PosVertex>::SquareLineStrip;
    delete SimpleMesh<PosVertex>::CircleLineStrip;

    delete SimpleMesh<TexVertex>::Quad;
}

} // namespace dm
