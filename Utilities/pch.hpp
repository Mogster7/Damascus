
#include <iostream>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <stdexcept>
#include <cstdlib>
#include <optional>
#include <memory>
#include <vector>
#include <array>
#include <algorithm>
#include <string>
#include <string_view>

#ifndef NDEBUG
#   define ASSERT(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            std::terminate(); \
        } \
    } while (false)
#else
#   define ASSERT(condition, message) do { } while (false)
#endif


#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VK_USE_PLATFORM_WIN32_KHR 1
#define NOMINMAX 1
#include <vulkan/vulkan.hpp>

// Other
#include "Utilities.h"

// RENDERING 
#include "RenderingDefines.hpp"
#include "RenderingStructures.hpp"

