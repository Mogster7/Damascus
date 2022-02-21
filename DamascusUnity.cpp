#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

// Other
#include "Utilities.h"
#include "Sorting/RenderSortKey.h"

// Rendering
#include "RenderingDefines.hpp"
#include "RenderingStructures.hpp"

// clang-format off
#include "Renderer/Renderer.cpp"
#include "Sorting/RenderSortKey.cpp"
#include "Window/Window.cpp"
#include "InternalStructures/Model.cpp"
#include "InternalStructures/Vertex.cpp"
#include "InternalStructures/Mesh.cpp"
#include "InternalStructures/Buffer.cpp"
#include "InternalStructures/Device.cpp"
#include "InternalStructures/PhysicalDevice.cpp"
#include "InternalStructures/Instance.cpp"
#include "InternalStructures/Swapchain.cpp"
#include "InternalStructures/ImageView.cpp"
#include "InternalStructures/RenderPass.cpp"
#include "InternalStructures/ShaderModule.cpp"
#include "InternalStructures/Pipeline.cpp"
#include "InternalStructures/Texture.cpp"
#include "InternalStructures/CommandBuffer.cpp"
#include "InternalStructures/CommandPool.cpp"
#include "InternalStructures/Semaphore.cpp"
#include "InternalStructures/Fence.cpp"
#include "InternalStructures/Descriptors.cpp"
#include "InternalStructures/Image.cpp"
#include "InternalStructures/FrameBufferAttachment.cpp"
#include "Camera/Camera.cpp"
#include "Primitives/Primitives.cpp"
// clang-format on
