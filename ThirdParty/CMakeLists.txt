cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

# Disable unnecessary glfw builds
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# Find vulkan
find_package(Vulkan 1.2.176)
set(Vulkan_LIBRARIES ${Vulkan_LIBRARY})
set(Vulkan_INCLUDE_DIRS ${Vulkan_INCLUDE_DIR})

if(Vulkan_FOUND)
    message(STATUS, " Vulkan found")
else (Vulkan_FOUND)
    message(FATAL_ERROR, ", Could not find Vulkan, can't build")
endif(Vulkan_FOUND)


set(SRC
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_tables.cpp
        imgui/backends/imgui_impl_vulkan.cpp
        imgui/backends/imgui_impl_glfw.cpp
        )
add_library(
        ThirdParty ${SRC}
)

set(VULKAN_DIR VulkanSDK)
set(GLFW_INCLUDE_DIR glfw/include/GLFW)
set(IMGUI_INCLUDE_DIR imgui)
set(IMGUI_INCLUDE_BACKENDS_DIR imgui/backends)

target_include_directories(ThirdParty PUBLIC
        VulkanMemoryAllocator/src
        vulkan-hpp
        ${IMGUI_INCLUDE_DIR}
        ${IMGUI_INCLUDE_BACKENDS_DIR}
        ${GLFW_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${Vulkan_INCLUDE_DIRS}
        )

target_link_directories(ThirdParty PUBLIC
        ${GLFW_STATIC_LIBRARIES}
        ${Vulkan_INCLUDE_DIRS}
        )

target_link_libraries(ThirdParty PUBLIC glfw ${Vulkan_LIBRARIES})

