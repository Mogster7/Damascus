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

	RenderComponent& operator=(RenderComponent& other) = delete;
	RenderComponent(RenderComponent& other) = delete;

	RenderComponent(RenderComponent&& other) noexcept = default;
	RenderComponent& operator=(RenderComponent&& other) noexcept = default;
	~RenderComponent() noexcept = default;

	RenderComponent(Mesh<VertexType>&& other) noexcept
		: mesh(std::move(other))
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

