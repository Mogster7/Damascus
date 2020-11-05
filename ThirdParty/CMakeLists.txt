cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

add_library(ThirdParty
    INTERFACE
    )

target_include_directories(ThirdParty INTERFACE
    VulkanMemoryAllocator/src
    EASTL/test/packages/EABase/include/Common
    EASTL/include
    )


target_link_directories(ThirdParty INTERFACE EASTL/lib)
target_link_libraries(ThirdParty INTERFACE EASTL)

