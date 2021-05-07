cmake_minimum_required(VERSION 3.16)
project(Vulkan)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(    source_list
        Application/Scenes/DemoScene.cpp
        Application/SpatialPartitioning/SpatialPartitioning.hpp
        Application/SpatialPartitioning/BSP/BSP.hpp
        Application/SpatialPartitioning/Octree/Octree.hpp
        main.cpp)

set(FRAMEWORK_INCLUDE_DIR ${CMAKE_SOURCE_DIR}\\Framework\\)
set(INCLUDES
        .\\Application Assets Assets\\Shaders Assets\\Models
    ${CMAKE_SOURCE_DIR} 
    ${GLM_INCLUDE_DIR} 
    ${GLFW_INCLUDE_DIR}
    Utilities)

set(LINK_DIRS ThirdParty Utilities)
set(LINK_LIBS ThirdParty Framework Utilities)
if(WIN32)
    set(POST_COMMAND COMMAND cmd /c ${CMAKE_SOURCE_DIR}/Assets/Shaders/compile_win.bat
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/Assets/Models $<TARGET_FILE_DIR:DemoScene>/Assets/Models
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/Assets/Shaders $<TARGET_FILE_DIR:DemoScene>/Assets/Shaders)
else()
   set(POST_COMMAND COMMAND ${CMAKE_SOURCE_DIR}/Assets/Shaders/compile_linux.sh
            #COMMAND ${CMAKE_COMMAND} -E copy_directory
            # ${CMAKE_SOURCE_DIR}/Assets/Models $<TARGET_FILE_DIR:DemoScene>/Assets/Models
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/Assets/Shaders $<TARGET_FILE_DIR:DemoScene>/Assets/Shaders)
endif()

add_executable(DemoScene ${source_list})

add_subdirectory(Framework)
add_subdirectory(Utilities)
add_subdirectory(ThirdParty)
target_include_directories(DemoScene PUBLIC ${INCLUDES})
target_link_directories(DemoScene PUBLIC ${LINK_DIRS})
target_link_libraries(DemoScene PUBLIC ${LINK_LIBS})
target_precompile_headers(DemoScene REUSE_FROM Utilities)
target_precompile_headers(Framework REUSE_FROM Utilities)
add_custom_command(TARGET DemoScene POST_BUILD ${POST_COMMAND})

