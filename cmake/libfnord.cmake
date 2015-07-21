include(lib/libfnord/cmake/CFlags.cmake)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/..)
include_directories(${FNORD_INCLUDE})
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FNORD_LDFLAGS}")
