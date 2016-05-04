set(CMAKE_CXX_FLAGS "-std=c++0x -ftemplate-depth=500 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wdelete-non-virtual-dtor ${CMAKE_CXX_FLAGS} -Wno-predefined-identifier-outside-function")
set(CMAKE_C_FLAGS "-std=c11 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -Wall -pedantic ${CMAKE_C_FLAGS}")

set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -lpthread")

set(CMAKE_C_FLAGS_DEBUG "-g ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_DEBUG "-g ${CMAKE_CXX_FLAGS_DEBUG}")
#set(CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address ${CMAKE_CXX_FLAGS_DEBUG}")
#set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-fsanitize=address ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")

# OSX FLAGS
if(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ ")

# LINUX FLAGS
else()
  set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl -static-libstdc++ -static-libgcc")
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  set(BUILD_SHARED_LIBRARIES OFF)
  set(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS)       # remove -Wl,-Bdynamic
  set(CMAKE_EXE_LINK_DYNAMIC_CXX_FLAGS)
  set(CMAKE_SHARED_LIBRARY_C_FLAGS)         # remove -fPIC
  set(CMAKE_SHARED_LIBRARY_CXX_FLAGS)
  set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)    # remove -rdynamic
  set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
endif()
