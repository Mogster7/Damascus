//------------------------------------------------------------------------------
//
// File Name:	Mesh.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/8/2020 
//
//------------------------------------------------------------------------------
#pragma once

struct Model
{
    operator glm::mat4() { return model; }
    glm::mat4 model = glm::mat4(1.0f);
};

class Mesh
{
public:
    void Create(Device& device, const eastl::vector<Vertex>& vertices, 
        const eastl::vector<uint32_t>& indices);
    void Destroy();

    void SetModel(const glm::mat4& model) { m_Model.model = model; }
    const Model& GetModel() const { return m_Model; }

    int GetIndexCount() const { return m_IndexBuffer.GetIndexCount(); }
    Buffer GetIndexBuffer() const { return m_IndexBuffer; }
    void DestroyIndexBuffer() { m_IndexBuffer.Destroy(); }

    int GetVertexCount() const { return m_VertexBuffer.GetVertexCount(); }
    Buffer GetVertexBuffer() const { return m_VertexBuffer; }
    void DestroyVertexBuffer() { m_VertexBuffer.Destroy(); }

private:
    Model m_Model = {};

    VertexBuffer m_VertexBuffer = {};
    IndexBuffer m_IndexBuffer = {};

};




