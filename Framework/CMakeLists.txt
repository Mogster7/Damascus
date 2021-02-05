cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(FRAMEWORK_SRC 
    RenderingContext/RenderingContext.cpp
    Window/Window.cpp
    InternalStructures/RenderingDefines.hpp
    InternalStructures/RenderingStructures.hpp
    InternalStructures/Model.cpp
    InternalStructures/Vertex.cpp
    InternalStructures/Mesh.cpp
    InternalStructures/Buffer.cpp
    InternalStructures/Device.cpp
    InternalStructures/PhysicalDevice.cpp
    InternalStructures/Instance.cpp
    InternalStructures/Swapchain.cpp
    InternalStructures/ImageView.cpp
    InternalStructures/RenderPass.cpp
    InternalStructures/ShaderModule.cpp
    InternalStructures/Pipeline.cpp
    InternalStructures/Framebuffer.cpp
    InternalStructures/CommandPool.cpp
    InternalStructures/Semaphore.cpp
    InternalStructures/Fence.cpp
    InternalStructures/Descriptors.cpp
    InternalStructures/Image.cpp
    InternalStructures/DepthBuffer.cpp
 "InternalStructures/Texture.cpp" "Camera/Camera.cpp" "Camera/Camera.h")


add_library(Framework
    ${FRAMEWORK_SRC}
     )


set(LIBS
    glfw3
    vulkan-1
)

target_include_directories(Framework PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${VULKAN_INCLUDE_DIR} 
    ${GLFW_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/Utilities
    RenderingContext
    Structures
    Window
)

target_link_directories(Framework PUBLIC
    ${VULKAN_LIB_DIR} 
    ${CMAKE_SOURCE_DIR}/Utilities
    ${GLFW_LIB_DIR}
    )

target_link_libraries(Framework Utilities ${LIBS})
