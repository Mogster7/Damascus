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
        InternalStructures/Framebuffer.cpp
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
        Overlay/Overlay.cpp
        Object/Object.cpp
        Overlay/EditorWindow.cpp
        Overlay/EditorBlock.cpp
        Overlay/Blocks/StatsEditorBlock.cpp
        Overlay/Blocks/EntityEditorBlock.cpp
        Collider/Collider.cpp
        Primitives/Primitives.cpp
        ECS/ECS.cpp
        ECS/Components/Transform/TransformComponent.cpp
        ECS/Components/Transform/TransformComponentSystem.cpp
        ECS/Components/Render/RenderComponent.cpp
        ECS/Components/Render/RenderComponentSystem.cpp
        ECS/Components/Physics/PhysicsComponent.cpp
        ECS/Components/Physics/PhysicsComponentSystem.cpp
        Job/Job.cpp)


add_library(Framework
        ${FRAMEWORK_SRC})


set(LIBS
        ThirdParty
        Utilities
        )

target_include_directories(Framework PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Utilities
        ${CMAKE_SOURCE_DIR}/ThirdParty
        RenderingContext
        InternalStructures
        Window
        )

target_link_directories(Framework PUBLIC
        ${CMAKE_SOURCE_DIR}/Utilities
        ${CMAKE_SOURCE_DIR}/ThirdParty
        )

target_link_libraries(Framework ${LIBS})
