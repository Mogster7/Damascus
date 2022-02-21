cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(DAMASCUS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(FRAMEWORK_INCLUDE_DIR ${DAMASCUS_DIR}\\Framework\\)
set(LINK_DIRS ThirdParty Utilities)

add_library(Damascus DamascusUnity.cpp)

add_subdirectory(ThirdParty)
add_subdirectory(Utilities)
target_include_directories(Damascus PUBLIC Include ${CMAKE_CURRENT_SOURCE_DIR} Framework)
target_link_libraries(Damascus PUBLIC DamascusThirdParty DamascusUtilities)

