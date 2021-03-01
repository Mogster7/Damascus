cmake_minimum_required(VERSION 3.16)
project(Vulkan)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(
    source_list
    main.cpp)

set(FRAMEWORK_INCLUDE_DIR ${CMAKE_SOURCE_DIR}\\Framework\\)
set(INCLUDES 
    ${CMAKE_SOURCE_DIR} 
    ${GLM_INCLUDE_DIR} 
    ${GLFW_INCLUDE_DIR}
    Utilities)
set(LINK_DIRS ThirdParty Utilities)
set(LINK_LIBS ThirdParty Framework Utilities)
set(POST_COMMAND COMMAND cmd /c ${CMAKE_SOURCE_DIR}/Assets/Shaders/compile.bat
                   #COMMAND ${CMAKE_COMMAND} -E copy_directory
                   #    ${CMAKE_SOURCE_DIR}/Assets/Textures $<TARGET_FILE_DIR:Collision>/Assets/Textures
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/Assets/Shaders $<TARGET_FILE_DIR:Collision>/Assets/Shaders)

#add_executable(Deferred ${source_list} Demos/Deferred.cpp)
#target_link_directories(Deferred PUBLIC ${LINK_DIRS})
#target_include_directories(Deferred PUBLIC ${INCLUDES})
#add_custom_command(TARGET Deferred POST_BUILD ${POST_COMMAND})
#target_precompile_headers(Deferred REUSE_FROM Utilities)
#target_link_libraries(Deferred PUBLIC ${LINK_LIBS})

add_executable(Collision ${source_list} Demos/Collision.cpp)
add_subdirectory(Framework)
add_subdirectory(Utilities)
add_subdirectory(ThirdParty)
target_include_directories(Collision PUBLIC ${INCLUDES})
target_link_directories(Collision PUBLIC ${LINK_DIRS})
target_link_libraries(Collision PUBLIC ${LINK_LIBS})
target_precompile_headers(Collision REUSE_FROM Utilities)
target_precompile_headers(Framework REUSE_FROM Utilities)
add_custom_command(TARGET Collision POST_BUILD ${POST_COMMAND})

