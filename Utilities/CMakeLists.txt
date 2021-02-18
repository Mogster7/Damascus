add_library(Utilities
    Utilities.cpp
    
    )

    
target_include_directories(Utilities PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GLM_INCLUDE_DIR}
    ${VULKAN_INCLUDE_DIR}
    ${VMA_INCLUDE_DIR}
    ${FRAMEWORK_INCLUDE_DIR}
    ${IMGUI_INCLUDE_DIR}
    ${IMGUI_INCLUDE_BACKENDS_DIR}
    )


target_link_libraries(Utilities PUBLIC ThirdParty)
target_link_directories(Utilities PUBLIC ThirdParty)
target_precompile_headers(Utilities PUBLIC pch.hpp)

