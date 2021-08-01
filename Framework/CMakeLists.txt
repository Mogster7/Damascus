cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(FRAMEWORK_SRC
        RenderingContext/RenderingContext.cpp
        Window/Window.cpp
        RenderingDefines.hpp
        RenderingStructures.hpp
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
        InternalStructures/FrameBuffer.cpp
        InternalStructures/CommandBuffer.cpp
        InternalStructures/CommandPool.cpp
        InternalStructures/Semaphore.cpp
        InternalStructures/Fence.cpp
        InternalStructures/Descriptors.cpp
        InternalStructures/Image.cpp
        InternalStructures/FrameBufferAttachment.cpp
        InternalStructures/Texture.cpp
        InternalStructures/Shader.cpp
        InternalStructures/RenderQueue.cpp
        InternalStructures/CommandBuffer.h
        InternalStructures/Sampler.h
        Camera/Camera.cpp
        Primitives/Primitives.cpp
        )


add_library(Framework
        ${FRAMEWORK_SRC})



add_dependencies(Framework DamascusUtilities ThirdParty)
set(LIBS
        ThirdParty
        DamascusUtilities
        )


target_include_directories(Framework PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${DAMASCUS_DIR}/Utilities
        ${DAMASCUS_DIR}/ThirdParty
        RenderingContext
        InternalStructures
        Window
        )

target_link_directories(Framework PUBLIC
        ${DAMASCUS_DIR}/Utilities
        ${DAMASCUS_DIR}/ThirdParty
        )

target_link_libraries(Framework ${LIBS})
