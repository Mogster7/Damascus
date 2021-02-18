//------------------------------------------------------------------------------
//
// File Name:	RenderingContext.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/5/2020 
//
//------------------------------------------------------------------------------
#pragma once


struct RenderingContext
{    
    RenderingContext() = default;
    ~RenderingContext() = default;

    // Must be defined by a compilation unit for each demo
    static RenderingContext& Get();
    
    // Draw
    virtual void Draw() = 0;

     // Update
    virtual void Update();

    // Init functions
    virtual void Initialize(std::weak_ptr<Window> winHandle, bool enabledOverlay = true);

    virtual void Destroy();

    Instance instance = {};
    PhysicalDevice physicalDevice = {};
    Device device = {};
    Overlay overlay = {};
    bool enabledOverlay = false;

    uint32_t currentFrame = 0;
    float dt = 0.0f;


    std::weak_ptr<Window> window;
};





