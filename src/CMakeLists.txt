cmake_minimum_required(VERSION 2.8.8)
project(analyticsd)

# INCLUDES
set(MODULE_INCLUDE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/src/analytics
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(${MODULE_INCLUDE})
set(ANALYTICSD_INCLUDE ${MODULE_INCLUDE} PARENT_SCOPE)

# DEPENDENCIES
find_package(Threads)
add_subdirectory(src/customerdb)
add_subdirectory(src/analytics)
add_subdirectory(src/logjoin)
