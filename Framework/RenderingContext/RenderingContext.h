//------------------------------------------------------------------------------
//
// File Name:	Renderer.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/5/2020 
//
//------------------------------------------------------------------------------
#pragma once

namespace vk
{
    class CommandPool;
    class Queue;
}

class Window;
class QueueFamilyIndices;
class Vertex;
struct RenderingContext_Impl;

class RenderingContext
{

public:
    inline static float dt = 0.0f;
    inline static float time = 0.0f;

    static void Initialize(std::weak_ptr<Window> window);
    static void Update();
    static void Destroy();

    static void MakeMesh();
    static void DrawFrame();
    static Instance& GetInstance();
    static Device& GetDevice();
    static PhysicalDevice& GetPhysicalDevice();

    // Pool/queue
    static vk::CommandPool GetGraphicsPool();
    static vk::Queue GetGraphicsQueue();
    

protected:
    static RenderingContext_Impl* impl;

    friend class Window;
};





