cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(INCLUDES
        ${CMAKE_SOURCE_DIR}
        ${GLM_INCLUDE_DIR}
        ${GLFW_INCLUDE_DIR}
        ThirdParty
        Utilities)

set(BULKAN_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(FRAMEWORK_INCLUDE_DIR ${BULKAN_DIR}\\Framework\\)
set(LINK_DIRS ThirdParty Utilities)
set(LINK_LIBS ThirdParty Framework Utilities)

add_library(Bulkan INTERFACE)

add_subdirectory(Framework)
add_subdirectory(Utilities)
add_subdirectory(ThirdParty)
target_include_directories(Bulkan INTERFACE ${INCLUDES})
target_link_directories(Bulkan INTERFACE ${LINK_DIRS})
target_link_libraries(Bulkan INTERFACE ${LINK_LIBS})
target_precompile_headers(Bulkan REUSE_FROM Utilities)
target_precompile_headers(Framework REUSE_FROM Utilities)

