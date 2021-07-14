
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <stdexcept>
#include <set>
#include <cstdlib>
#include <thread>
#include <optional>
#include <memory>
#include <vector>
#include <array>
#include <queue>
#include <algorithm>
#include <stack>
#include <tuple>
#include <string>
#include <string_view>
#include <type_traits>

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
#   define ASSERT(condition, message) do { } while(false)
#endif


#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define NOMINMAX 1
#include <vulkan/vulkan.h>
#include <vulkan.hpp>

// Other
#include "Utilities.h"

// Rendering
#include "RenderingDefines.hpp"
#include "RenderingStructures.hpp"





