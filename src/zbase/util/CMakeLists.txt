project(FNORD_FTS)

find_package(Threads)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static")

include_directories(../)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE FNORD_FTS_SOURCES
  Analyzer.cc
  SimpleTokenizer.cc
  GermanStemmer.cc
  Hunspell.cc
  StopwordDictionary.cc
  SynonymDictionary.cc)

set(FNORD_FTS_LIBS
    ${CMAKE_THREAD_LIBS_INIT}
    z
    PARENT_SCOPE)

set(FNORD_FTS_INCLUDE
    PARENT_SCOPE)

add_library(nlputil OBJECT ${FNORD_FTS_SOURCES})
