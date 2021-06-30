//------------------------------------------------------------------------------
//
// File Name:	Instance.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/18/2020 
//
//------------------------------------------------------------------------------
#pragma once

//namespace vk
//{
//    class SurfaceKHR;
//    class DebugUtilsMessengerEXT;
//    class Instance;
//    class DynamicLoader;
//}

class Window;

class Instance : public vk::Instance
{
    std::weak_ptr<Window> window;
    vk::SurfaceKHR surface;
    vk::DebugUtilsMessengerEXT debugMessenger = {};
    vk::DynamicLoader dynamicLoader = {};

public:
    Instance() = default;
    void Create();
    void CreateSurface(std::weak_ptr<Window> winHandle);
    void Destroy();
    ~Instance() = default;
    UNDERLYING_CONVERSION(Instance,)
    DERIVED_GETTER(Instance, Instance,)

    vk::SurfaceKHR GetSurface() const { return surface; }
    std::weak_ptr<Window> GetWindow() const { return window; }

private:
    static void PopulateDebugCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo);
    void ConstructDebugMessenger();

};




