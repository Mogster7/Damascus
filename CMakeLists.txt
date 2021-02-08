cmake_minimum_required(VERSION 3.16)
project(Vulkan)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(
    source_list
    main.cpp
     "Demos/deferred.cpp" "Demos/deferred.h")


set(VULKAN_DIR C:\\VulkanSDK\\1.2.162.0)
set(VULKAN_INCLUDE_DIR ${VULKAN_DIR}\\Include)
set(VULKAN_LIB_DIR ${VULKAN_DIR}\\Lib)
set(FRAMEWORK_INCLUDE_DIR ${CMAKE_SOURCE_DIR}\\Framework\\InternalStructures)

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

add_subdirectory(Framework)
add_subdirectory(Utilities)
add_subdirectory(ThirdParty)
target_link_libraries(${PROJECT_NAME} PUBLIC ThirdParty Framework Utilities)
target_precompile_headers(${PROJECT_NAME} REUSE_FROM Utilities)
target_precompile_headers(Framework REUSE_FROM Utilities)


add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                   COMMAND cmd /c ${CMAKE_SOURCE_DIR}/Assets/Shaders/compile.bat
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/Assets/Textures $<TARGET_FILE_DIR:${PROJECT_NAME}>/Assets/Textures
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/Assets/Shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/Assets/Shaders)
