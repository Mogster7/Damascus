cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

add_library(Framework
    Renderer/Renderer.cpp
    Window/Window.cpp
    Structures/RenderingStructures.hpp
    Structures/Vertex.cpp
    Structures/Mesh.cpp
    Structures/Buffer.cpp
)

set(LOCAL_LIB_DIR C:/Libraries)
set(VULKAN_DIR C:\\VulkanSDK\\1.2.135.0)
set(VULKAN_INCLUDE_DIR ${VULKAN_DIR}\\Include)
set(VULKAN_LIB_DIR ${VULKAN_DIR}\\Lib)

set(GLFW_INCLUDE_DIR ${LOCAL_LIB_DIR}/glfw/include/GLFW)
set(GLFW_LIB_DIR ${LOCAL_LIB_DIR}/glfw/lib-vc2019)


set(LIBS
    glfw3
    vulkan-1
)

target_include_directories(Framework PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} 
    ${VULKAN_INCLUDE_DIR} 
    ${GLFW_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/Utilities
    Renderer
    Structures
    Window
)

target_link_directories(Framework PUBLIC
    ${VULKAN_LIB_DIR} 
    ${CMAKE_SOURCE_DIR}/Utilities
    ${GLFW_LIB_DIR}
    )
target_link_libraries(Framework Utilities ${LIBS})
target_precompile_headers(Framework REUSE_FROM Utilities)
