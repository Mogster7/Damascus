cmake_minimum_required(VERSION 3.16)
project(Vulkan)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(
    source_list
    main.cpp
    )


set(VULKAN_DIR C:\\VulkanSDK\\1.2.162.0)
set(VULKAN_INCLUDE_DIR ${VULKAN_DIR}\\Include)
set(VULKAN_LIB_DIR ${VULKAN_DIR}\\Lib)

add_executable(${PROJECT_NAME} ${source_list})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR} 
    ${GLM_INCLUDE_DIR} 
    ${GLFW_INCLUDE_DIR}
    Utilities
    )

target_link_directories(${PROJECT_NAME} PUBLIC
    ThirdParty
    Utilities
    )

add_subdirectory(Utilities)
add_subdirectory(Framework)
add_subdirectory(ThirdParty)
target_link_libraries(${PROJECT_NAME} PUBLIC ThirdParty Utilities Framework)
target_precompile_headers(${PROJECT_NAME} REUSE_FROM Utilities)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/Shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/Shaders)
