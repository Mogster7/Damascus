cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})


# Find vulkan
find_package(Vulkan 1.2.182.0 REQUIRED)
if(Vulkan_FOUND)
    message("Vulkan Found!")
    message("Vulkan Directory: ${Vulkan_INCLUDE_DIR}")
else (Vulkan_FOUND)
    message(FATAL_ERROR " Could not find Vulkan, can't build")
endif(Vulkan_FOUND)

add_library(
        ThirdParty INTERFACE
)


target_include_directories(ThirdParty INTERFACE
        VulkanMemoryAllocator/src
        ${Vulkan_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${SDL2_INCLUDE_DIRS}
        )

target_link_directories(ThirdParty INTERFACE ${SDL2_INCLUDE_DIRS})

set(LINK_LIBS ${Vulkan_LIBRARIES} SDL2-static glm)

if (WIN32)
    if (MSVC)
        target_link_libraries(ThirdParty INTERFACE ${LINK_LIBS})
    else(MSVC)
        target_link_libraries(ThirdParty INTERFACE ${LINK_LIBS} imm32)
    endif(MSVC)
else(WIN32)
    target_link_libraries(ThirdParty INTERFACE ${LINK_LIBS})
endif(WIN32)

