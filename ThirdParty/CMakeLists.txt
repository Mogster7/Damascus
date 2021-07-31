cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

# Disable unnecessary glfw builds
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# Find vulkan
find_package(Vulkan 1.2.182.0)
if(Vulkan_FOUND)
    message("Vulkan Found!")
    message("Vulkan Directory: ${Vulkan_INCLUDE_DIR}")
else (Vulkan_FOUND)
    message(FATAL_ERROR " Could not find Vulkan, can't build")
endif(Vulkan_FOUND)

add_library(
        ThirdParty INTERFACE
)


set(GLFW_INCLUDE_DIR glfw/include/GLFW)

target_include_directories(ThirdParty INTERFACE
        VulkanMemoryAllocator/src
        ${Vulkan_INCLUDE_DIRS}
        ${GLFW_INCLUDE_DIR}
        ${VULKAN_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        )

target_link_directories(ThirdParty INTERFACE
        ${GLFW_STATIC_LIBRARIES}
        ${Vulkan_INCLUDE_DIRS}
        )

if (WIN32)
    if (MSVC)
        target_link_libraries(ThirdParty INTERFACE glfw ${Vulkan_LIBRARIES})
    else(MSVC)
        target_link_libraries(ThirdParty INTERFACE glfw ${Vulkan_LIBRARIES} imm32)
    endif(MSVC)
else(WIN32)
    target_link_libraries(ThirdParty INTERFACE glfw ${Vulkan_LIBRARIES})
endif(WIN32)

