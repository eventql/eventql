
# require a C++ compiler that at least the currently latest
# Ubuntu LTS release is supporting
if(APPLE)
  set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++")
else()
  set(CMAKE_CXX_FLAGS "-std=c++11")
endif()

# be more sensible on compiler warnings
add_definitions(-Wall -Wno-variadic-macros)

# temporary workaround for: http://gcc.gnu.org/bugzilla/show_bug.cgi?id=56627
add_definitions(-Wno-mismatched-tags)

# we need the following definitions in order to get some special
# OS-level features like posix_fadvise() or readahead().
add_definitions(-DXOPEN_SOURCE=600)
add_definitions(-D_GNU_SOURCE)

# ISO C99: explicitely request format specifiers
add_definitions(-D__STDC_FORMAT_MACROS)

# enforce 64bit i/o operations, even on 32bit platforms
add_definitions(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGE_FILES)
