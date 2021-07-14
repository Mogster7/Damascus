//------------------------------------------------------------------------------
//
// File Name:	Instance.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/18/2020
//
//------------------------------------------------------------------------------
#include "RenderingStructures.hpp"
#include "Window.h"
#include "Instance.h"

#include <glfw3.h>
#include <vector>

namespace bk {

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};


bool checkValidationLayerSupport()
{
	std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}


std::vector<const char*> GetRequiredExtensions()
{
	unsigned glfwExtensionCount = 0;
	const char** requiredExtensions;
	requiredExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(requiredExtensions, requiredExtensions + glfwExtensionCount);

	if (enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

void Instance::Create()
{
	auto vkGetInstanceProcAddr = dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	if (enableValidationLayers)
	{
		assert(checkValidationLayerSupport());
	}

	vk::ApplicationInfo appInfo("Hello Triangle", VK_MAKE_VERSION(1, 0, 0),
								"No Engine", VK_MAKE_VERSION(1, 0, 0),
								VK_API_VERSION_1_0);
	vk::InstanceCreateInfo createInfo({}, &appInfo);

	auto requiredExtensions = GetRequiredExtensions();

	std::cout << "Required Extensions: \n";
	for (const auto& extension : requiredExtensions)
	{
		std::cout << '\t' << extension << '\n';
	}
	std::cout << '\n';
	createInfo.enabledExtensionCount = (unsigned) requiredExtensions.size();
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();


	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();


		vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		PopulateDebugCreateInfo(debugCreateInfo);
		createInfo.pNext = (vk::DebugUtilsMessengerCreateInfoEXT * ) & debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	std::vector<vk::ExtensionProperties> extensions = vk::enumerateInstanceExtensionProperties();

	std::cout << "Available Extensions: \n";

	for (const auto& extension : extensions)
	{
		std::cout << '\t' << extension.extensionName << '\n';
	}

	std::cout << VK_HEADER_VERSION << std::endl;

	ASSERT_VK(vk::createInstance(&createInfo, nullptr, this));

	VULKAN_HPP_DEFAULT_DISPATCHER.init(VkType());

	ConstructDebugMessenger();
}

void Instance::CreateSurface(std::weak_ptr<Window> winHandle)
{
	window = winHandle;
	if (glfwCreateWindowSurface((VkInstance) (*this), window.lock()->GetHandle(), nullptr, (VkSurfaceKHR*) &surface) !=
		VK_SUCCESS)
	{
		ASSERT(false, "Failed to create window surface");
	}
}

static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
	vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	vk::DebugUtilsMessageTypeFlagsEXT messageType,
	const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
)
{
	if (messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose)
	{
		return VK_FALSE;
	}

	std::cerr << "validation layer: " << pCallbackData->pMessage << '\n' << std::endl;

	return VK_FALSE;
}


void Instance::PopulateDebugCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = vk::DebugUtilsMessengerCreateInfoEXT(
		{},

		vk::DebugUtilsMessageSeverityFlagsEXT(VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
											  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
											  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT),

		vk::DebugUtilsMessageTypeFlagsEXT(VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
										  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
										  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT),

		(PFN_vkDebugUtilsMessengerCallbackEXT) DebugCallback
	);
}

void Instance::ConstructDebugMessenger()
{
	if (!enableValidationLayers)
	{
		return;
	}

	vk::DebugUtilsMessengerCreateInfoEXT info;
	PopulateDebugCreateInfo(info);

	ASSERT_VK(createDebugUtilsMessengerEXT(&info, nullptr, &debugMessenger));
}

Instance::~Instance() noexcept
{
	if (!created)
	{
		return;
	}

	if (enableValidationLayers)
	{
		destroyDebugUtilsMessengerEXT(debugMessenger, nullptr);
	}

	destroySurfaceKHR(surface);
	destroy();
}


}