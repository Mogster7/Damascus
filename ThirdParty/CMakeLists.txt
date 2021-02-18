cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(LIBS
    glfw3
    vulkan-1
    )
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
set(VULKAN_INCLUDE_DIR VulkanSDK/Include)
set(VULKAN_LIB_DIR VulkanSDK/Lib)
set(GLFW_INCLUDE_DIR_EXT GLFW/include/GLFW)
set(GLFW_INCLUDE_DIR GLFW/include/)
set(GLFW_LIB_DIR GLFW/lib)
set(IMGUI_INCLUDE_DIR imgui)
set(IMGUI_INCLUDE_BACKENDS_DIR imgui/backends)

target_include_directories(ThirdParty PUBLIC
    VulkanMemoryAllocator/src
    ${IMGUI_INCLUDE_DIR}
    ${IMGUI_INCLUDE_BACKENDS_DIR}
    ${GLFW_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR_EXT}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${VULKAN_INCLUDE_DIR}
    )

target_link_directories(ThirdParty PUBLIC
    ${GLFW_LIB_DIR}
    ${VULKAN_LIB_DIR}
    )
target_link_libraries(ThirdParty PUBLIC ${LIBS})

