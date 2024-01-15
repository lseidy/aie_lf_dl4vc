cmake_minimum_required (VERSION 2.6)

# Maps to a solution file (EVEY.sln).
project (EVEY)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set compiler flags and options. 
if( MSVC )
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3")
elseif( UNIX OR MINGW )
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release")
    endif()
    message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")    

    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set(OPT_LV "O0")
    else()
        set(OPT_LV "O3")
    endif()
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -${OPT_LV} -fomit-frame-pointer -Wall -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-variable -Wno-attributes -Werror -Wno-strict-overflow -Wno-unknown-pragmas -Wno-stringop-overflow -std=c99 -fms-extensions")
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-pointer-sign")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lm -lstdc++")
endif()

# Command to output information to the console
message ("c Flags: " ${CMAKE_C_FLAGS})
message ("linker Flags: " ${CMAKE_EXE_LINKER_FLAGS})

# Sub-directories where more CMakeLists.txt exist
add_subdirectory(src)
add_subdirectory(app)
