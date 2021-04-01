#pragma once

template <class VertexType>
class RenderComponent
{
public:
	RenderComponent(Mesh<VertexType>& mesh)
		:
		mesh(std::move(mesh))
	{
	}

	Mesh<VertexType> mesh;
};


// Derivatives for layering
class DeferredRenderComponent : public RenderComponent<Vertex>
{

};

class ForwardRenderComponent : public RenderComponent<Vertex>
{

};

class PostRenderComponent : public RenderComponent<TexVertex>
{

};
