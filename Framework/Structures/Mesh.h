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
    void Create(Device& device, const std::vector<Vertex>& vertices, 
        const std::vector<uint32_t>& indices);
    void Destroy();

    void SetModel(const glm::mat4& model) { m_model = model; }
    const glm::mat4& GetModel() const { return m_model; }

    int GetIndexCount() const { return m_indexBuffer.GetIndexCount(); }
    Buffer GetIndexBuffer() const { return m_indexBuffer; }
    void DestroyIndexBuffer() { m_indexBuffer.Destroy(); }

    int GetVertexCount() const { return m_vertexBuffer.GetVertexCount(); }
    Buffer GetVertexBuffer() const { return m_vertexBuffer; }
    void DestroyVertexBuffer() { m_vertexBuffer.Destroy(); }

private:
    glm::mat4 m_model = glm::mat4(1.0f);

    VertexBuffer m_vertexBuffer = {};
    IndexBuffer m_indexBuffer = {};

};




