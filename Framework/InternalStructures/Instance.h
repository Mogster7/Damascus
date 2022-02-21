//------------------------------------------------------------------------------
//
// File Name:	Instance.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/18/2020 
//
//------------------------------------------------------------------------------
#pragma once
namespace dm
{
class Window;
class Renderer;

class Instance : public IVulkanType<vk::Instance>, public IOwned<Renderer>
{

public:
	DM_TYPE_VULKAN_OWNED_BODY(Instance, IOwned<Renderer>)

	void Create(std::weak_ptr<dm::Window> inWindow);
    void Destroy();
	~Instance() noexcept override;

	std::weak_ptr<Window> window;
	vk::SurfaceKHR surface;
	vk::DebugUtilsMessengerEXT debugMessenger = {};
	vk::DynamicLoader dynamicLoader = {};


private:
	static void PopulateDebugCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo);
  [[nodiscard]] std::vector<const char*> GetRequiredExtensions() const;

	void ConstructDebugMessenger();


};

}