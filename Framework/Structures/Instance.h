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

class Instance : public eastl::safe_object, public vk::Instance
{
    eastl::weak_ptr<Window> m_Window;
    vk::SurfaceKHR m_Surface;
    vk::DebugUtilsMessengerEXT m_DebugMessenger = {};
    vk::DynamicLoader m_DynamicLoader = {};

public:
    Instance() = default;
    void Create();
    void CreateSurface(eastl::weak_ptr<Window> winHandle);
    void Destroy();
    ~Instance() = default;

    vk::SurfaceKHR GetSurface() const { return m_Surface; }
    eastl::weak_ptr<Window> GetWindow() const { return m_Window; }

private:
    static void PopulateDebugCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo);
    void ConstructDebugMessenger();

};




