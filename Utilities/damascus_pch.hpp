
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <set>
#include <cstdlib>
#include <thread>
#include <optional>
#include <memory>
#include <vector>
#include <unordered_map>
#include <array>
#include <queue>
#include <algorithm>
#include <stack>
#include <tuple>
#include <string>
#include <string_view>
#include <type_traits>

#ifndef NDEBUG
#   define DM_ASSERT_MSG(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            std::terminate(); \
        } \
    } while (false)

#   define DM_ASSERT(condition) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << std::endl; \
            std::terminate(); \
        } \
    } while (false)

#else
#   define DM_ASSERT(condition) do { (void)(condition); } while (false)
#   define DM_ASSERT_MSG(condition, message) do { (void)(condition); } while(false)
#endif


#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

// Other
#include "Utilities.h"

// Rendering
#include "RenderingDefines.hpp"
#include "RenderingStructures.hpp"





