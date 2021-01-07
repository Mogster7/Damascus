cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

add_library(Framework
    Renderer/Renderer.cpp
    Window/Window.cpp
    Structures/RenderingDefines.hpp
    Structures/RenderingStructures.hpp
    Structures/Vertex.cpp
    Structures/Mesh.cpp
    Structures/Buffer.cpp
    Structures/Device.cpp
    Structures/PhysicalDevice.cpp
    Structures/Instance.cpp
    Structures/Swapchain.cpp
    Structures/ImageView.cpp
    Structures/RenderPass.cpp
    Structures/ShaderModule.cpp
    Structures/Pipeline.cpp
    Structures/Framebuffer.cpp
    Structures/CommandPool.cpp
    Structures/Semaphore.cpp
    Structures/Fence.cpp
    Structures/Descriptors.cpp
    Structures/Image.cpp
    Structures/DepthBuffer.cpp
    )


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
