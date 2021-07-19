add_library(DamascusUtilities
    Utilities.cpp
    
    )

    
target_include_directories(DamascusUtilities PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GLM_INCLUDE_DIR}
    ${VULKAN_INCLUDE_DIRS}
    ${VMA_INCLUDE_DIR}
    ${FRAMEWORK_INCLUDE_DIR}
    ${IMGUI_INCLUDE_DIR}
    ${IMGUI_INCLUDE_BACKENDS_DIR}
    )


target_link_libraries(DamascusUtilities PUBLIC ThirdParty)
target_link_directories(DamascusUtilities PUBLIC ThirdParty)
target_precompile_headers(DamascusUtilities PUBLIC damascus_pch.hpp)

