# based on https://github.com/quietust/dfhack-40d/blob/master/depends/protobuf
#
# License of dfhack
# https://github.com/peterix/dfhack
# Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any
# damages arising from the use of this software.
#
# Permission is granted to anyone to use this software for any
# purpose, including commercial applications, and to alter it and
# redistribute it freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must
# not claim that you wrote the original software. If you use this
# software in a product, an acknowledgment in the product documentation
# would be appreciated but is not required.
#
# 2. Altered source versions must be plainly marked as such, and
# must not be misrepresented as being the original software.
#
# 3. This notice may not be removed or altered from any source
# distribution.

EXECUTE_PROCESS(COMMAND ${CMAKE_C_COMPILER} -dumpversion
OUTPUT_VARIABLE GCC_VERSION)
STRING(REGEX MATCHALL "[0-9]+" GCC_VERSION_COMPONENTS ${GCC_VERSION})
LIST(GET GCC_VERSION_COMPONENTS 0 GCC_MAJOR)
LIST(GET GCC_VERSION_COMPONENTS 1 GCC_MINOR)
SET(HAVE_HASH_MAP 0)
SET(HASH_MAP_CLASS unordered_map)

#Check for all of the possible combinations of unordered_map and hash_map
FOREACH(header tr1/unordered_map unordered_map)
    FOREACH(namespace std::tr1 std )
        IF(HAVE_HASH_MAP EQUAL 0 AND NOT STL_HASH_OLD_GCC)
            CONFIGURE_FILE("${CMAKE_CURRENT_LIST_DIR}/TestHashMap.cc.in" "${CMAKE_CURRENT_BINARY_DIR}/TestHashMap.cc")
            TRY_COMPILE(HASH_MAP_COMPILE_RESULT ${PROJECT_BINARY_DIR}/CMakeTmp "${CMAKE_CURRENT_BINARY_DIR}/TestHashMap.cc")
            IF (HASH_MAP_COMPILE_RESULT AND HASH_MAP_RUN_RESULT EQUAL 1)
                SET(HASH_MAP_H <${header}>)
                STRING(REPLACE "map" "set" HASH_SET_H ${HASH_MAP_H})
                SET(HASH_NAMESPACE ${namespace})
                SET(HASH_MAP_CLASS unordered_map)
                SET(HASH_SET_CLASS unordered_set)
                SET(HAVE_HASH_MAP 1)
                SET(HAVE_HASH_SET 1)
            ENDIF()
        ENDIF()
    ENDFOREACH(namespace)
ENDFOREACH(header)
IF (HAVE_HASH_MAP EQUAL 0)
    SET(HASH_MAP_CLASS hash_map)
    FOREACH(header ext/hash_map hash_map)
        FOREACH(namespace __gnu_cxx "" std stdext)
            IF (HAVE_HASH_MAP EQUAL 0)
              CONFIGURE_FILE("${CMAKE_CURRENT_LIST_DIR}/TestHashMap.cc.in" "${CMAKE_CURRENT_BINARY_DIR}/TestHashMap.cc")
              TRY_COMPILE(HASH_MAP_COMPILE_RESULT ${PROJECT_BINARY_DIR}/CMakeTmp "${CMAKE_CURRENT_BINARY_DIR}/TestHashMap.cc")
                IF (HASH_MAP_COMPILE_RESULT)
                    SET(HASH_MAP_H <${header}>)
                    STRING(REPLACE "map" "set" HASH_SET_H ${HASH_MAP_H})
                    SET(HASH_NAMESPACE ${namespace})
                    SET(HASH_MAP_CLASS hash_map)
                    SET(HASH_SET_CLASS hash_set)
                    SET(HAVE_HASH_MAP 1)
                    SET(HAVE_HASH_SET 1)
                ENDIF()
            ENDIF()
        ENDFOREACH()
    ENDFOREACH()
ENDIF()

IF (HAVE_HASH_MAP EQUAL 0)
    MESSAGE(SEND_ERROR "Could not find a working hash map implementation. Please install GCC >= 4.4, and all necessary 32-bit C++ development libraries.")
ENDIF()
