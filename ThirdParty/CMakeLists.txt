cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})


# Find vulkan
find_package(Vulkan 1.2.182.0 REQUIRED)
if(Vulkan_FOUND)
    message("Vulkan Found!")
    message("Vulkan Directory: ${Vulkan_INCLUDE_DIRS}")
else (Vulkan_FOUND)
    message(FATAL_ERROR " Could not find Vulkan, can't build")
endif(Vulkan_FOUND)

if (UNIX)
    find_package(X11)
    if(X11_FOUND)
        message("X11 Found!")
    else ()
        message("no X11 cuh")
    endif()
    if(APPLE)
    endif()
endif()

add_library(
        DamascusThirdParty
        spirv_reflect/spirv_reflect.c
)

target_link_directories(DamascusThirdParty PUBLIC ${SDL2_INCLUDE_DIRS})

set(INCLUDE_DIRS VulkanMemoryAllocator/src
        ${Vulkan_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${SDL2_INCLUDE_DIRS}
        )

if (OS_Mac)
find_library(SDL2 sdl2)
find_package(SDL2 REQUIRED)

set(LINK_LIBS ${Vulkan_LIBRARIES} glm ${SDL2})
set(INCLUDE_DIRS ${INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})
else()
    set(LINK_LIBS "${Vulkan_LIBRARIES}" glm SDL2-static)
endif()

if (WIN32)
message("Damascus linking for Windows")
    target_include_directories(DamascusThirdParty PUBLIC ${INCLUDE_DIRS})
    target_link_libraries(DamascusThirdParty PUBLIC ${LINK_LIBS})
elseif(UNIX)
    message("Damascus linking for Linux")
    target_include_directories(DamascusThirdParty PUBLIC ${INCLUDE_DIRS} ${X11_INCLUDE_DIR})
    target_link_libraries(DamascusThirdParty PUBLIC ${LINK_LIBS} ${X11_LIBRARIES})
endif()

