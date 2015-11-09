include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

include(CortexCommon)

add_subdirectory(cortex-base)
add_subdirectory(cortex-http)
add_subdirectory(cortex-flow)
