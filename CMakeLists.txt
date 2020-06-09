cmake_minimum_required(VERSION 3.16)
project(Vulkan)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(
    source_list
    main.cpp
    )

add_executable(${PROJECT_NAME} ${source_list})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR} ${GLM_INCLUDE_DIR} ${GLFW_INCLUDE_DIR}
    Utilities
    )
target_link_directories(${PROJECT_NAME} PUBLIC
    Utilities
    )


add_subdirectory(Utilities)
add_subdirectory(Framework)
target_link_libraries(${PROJECT_NAME} PUBLIC Utilities Framework)
target_precompile_headers(${PROJECT_NAME} REUSE_FROM Utilities)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/Shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/Shaders)
