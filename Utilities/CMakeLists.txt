add_library(Utilities
    Utilities.cpp
    pch.cpp
    )

set(GLM_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/Libraries/glm)
set(VULKAN_DIR C:\\VulkanSDK\\1.2.135.0)
set(VULKAN_INCLUDE_DIR ${VULKAN_DIR}\\Include)


target_precompile_headers(Utilities PUBLIC pch.hpp)

target_include_directories(Utilities PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GLM_INCLUDE_DIR}
    ${VULKAN_INCLUDE_DIR}
    )
