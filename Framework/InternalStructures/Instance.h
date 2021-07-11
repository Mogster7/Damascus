//------------------------------------------------------------------------------
//
// File Name:	Instance.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/18/2020 
//
//------------------------------------------------------------------------------
#pragma once
namespace bk {
class Window;
class RenderingContext;

//BK_TYPE(Instance)
class Instance : public IVulkanType<vk::Instance>, public IOwned<RenderingContext>
{

public:
	BK_TYPE_VULKAN_OWNED_BODY(Instance, IOwned<RenderingContext>)

	void Create();
	void CreateSurface(std::weak_ptr<Window> winHandle);
	~Instance() noexcept;

	std::weak_ptr<Window> window;
	vk::SurfaceKHR surface;
	vk::DebugUtilsMessengerEXT debugMessenger = {};
	vk::DynamicLoader dynamicLoader = {};


private:
	static void PopulateDebugCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo);

	void ConstructDebugMessenger();


};

}