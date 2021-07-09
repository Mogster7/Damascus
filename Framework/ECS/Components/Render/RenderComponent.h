#pragma once

using namespace bk;

template <class VertexType>
class RenderComponent
{
public:
	RenderComponent() = default;
	RenderComponent(const std::string& path, Device& owner)
	{
		mesh = new Mesh<VertexType>();
		mesh->CreateModel(path, false);
	}
	RenderComponent(Mesh<VertexType>& other)
		: mesh(other)
	{
	}

	~RenderComponent()
	{
		//delete mesh;
	}

	Mesh<VertexType> mesh;
};


// Derivatives for layering / different passes
class DeferredRenderComponent : public RenderComponent<Vertex>
{

};

class ForwardRenderComponent : public RenderComponent<Vertex>
{

};

class PostRenderComponent : public RenderComponent<TexVertex>
{

};

class DebugRenderComponent : public RenderComponent<PosVertex>
{

};

