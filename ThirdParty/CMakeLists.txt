cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

add_library(
    ThirdParty INTERFACE
)

set(LIBS
    glfw3
    )

set(GLFW_INCLUDE_DIR GLFW/include/GLFW)
set(GLFW_LIB_DIR GLFW/lib)

target_include_directories(ThirdParty INTERFACE
    VulkanMemoryAllocator/src
    ${GLFW_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    )

target_link_directories(ThirdParty INTERFACE 
    ${GLFW_LIB_DIR}
    )
target_link_libraries(ThirdParty INTERFACE ${LIBS})

