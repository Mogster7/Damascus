add_library(Utilities
    Utilities.cpp
    pch.cpp
    )

set(GLM_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/ThirdParty/)
set(VULKAN_DIR C:\\VulkanSDK\\1.2.162.0)
set(VULKAN_INCLUDE_DIR ${VULKAN_DIR}\\Include)
set(RENDER_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/Framework/Structures)

set(LOCAL_LIB_DIR C:/Libraries)
set(EASTL_ROOT_DIR ${CMAKE_SOURCE_DIR}/ThirdParty/EASTL)
set(EABASE_ROOT_DIR ${CMAKE_SOURCE_DIR}/ThirdParty/EASTL/test/packages/EABase)
set(EASTL_LIB_DIR ${EASTL_ROOT_DIR}/lib)

    
#set(EASTL_LIBRARY ${EASTL_ROOT_DIR}/build/Debug/EASTL.lib optimized)
#add_custom_target(NatVis SOURCES ${EASTL_ROOT_DIR}/doc/EASTL.natvis)
target_include_directories(Utilities PUBLIC
    ${EASTL_ROOT_DIR}/include
    ${EABASE_ROOT_DIR}/include/Common
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GLM_INCLUDE_DIR}
    ${VULKAN_INCLUDE_DIR}
    ${VMA_INCLUDE_DIR}
    ${RENDER_INCLUDE_DIR}
    )


target_link_libraries(Utilities PUBLIC ThirdParty)
target_link_directories(Utilities PUBLIC ThirdParty)
target_precompile_headers(Utilities PUBLIC pch.hpp)

