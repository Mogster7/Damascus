#pragma once

template <class VertexType>
class RenderComponent
{
public:
	RenderComponent() = default;
	RenderComponent(const std::string& path, Device& owner)
	{
		mesh.CreateModel(path, false, owner);
	}

	RenderComponent(Mesh<VertexType>& mesh)
		:
		mesh(mesh)
	{
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

