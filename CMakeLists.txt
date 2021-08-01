cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(INCLUDES
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        Include
        Framework
        ThirdParty
        Utilities)

set(DAMASCUS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(FRAMEWORK_INCLUDE_DIR ${DAMASCUS_DIR}\\Framework\\)
set(LINK_DIRS ThirdParty Utilities)
set(LINK_LIBS ThirdParty Framework DamascusUtilities)

add_library(Damascus INTERFACE)

add_subdirectory(Framework)
add_subdirectory(Utilities)
add_subdirectory(ThirdParty)
target_include_directories(Damascus INTERFACE Include)
target_link_libraries(Damascus INTERFACE ${LINK_LIBS})

target_precompile_headers(Framework REUSE_FROM DamascusUtilities)

