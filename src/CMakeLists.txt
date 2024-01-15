# common library
set( LIB_NAME evey )

file (GLOB EVEY_INC "../inc/*.h") 
file (GLOB COMMON_LIB_FUNCTION_SRC "evey_*.c" "evey.c")
file (GLOB COMMON_LIB_FUNCTION_INC "evey_*.h")

add_library( ${LIB_NAME} STATIC ${EVEY_INC} ${COMMON_LIB_FUNCTION_SRC} ${COMMON_LIB_FUNCTION_INC} )

target_include_directories( ${LIB_NAME} PUBLIC . .. ../inc/)

set_target_properties(${LIB_NAME} PROPERTIES FOLDER lib
                                             ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/lib)

set( SSE ${BASE_INC_FILES} evey_inter.c evey_util.c evey_itdq.c)

if( UNIX OR MINGW )
  set_property( SOURCE ${SSE} APPEND PROPERTY COMPILE_FLAGS "-msse4.1" )
  target_link_libraries(${LIB_NAME} m)
endif()

# encdoer library
set( ENC_LIB_NAME eveye )

file (GLOB ENC_LIB_FUNCTION_SRC "eveye*.c")
file (GLOB ENC_LIB_FUNCTION_INC "eveye*.h")

include_directories (.)

add_library( ${ENC_LIB_NAME} STATIC ${ENC_LIB_FUNCTION_SRC} ${ENC_LIB_FUNCTION_INC} )

target_compile_definitions( ${ENC_LIB_NAME} PUBLIC )
target_include_directories( ${ENC_LIB_NAME} PUBLIC . )
target_link_libraries( ${ENC_LIB_NAME} evey)

set_target_properties(${ENC_LIB_NAME} PROPERTIES FOLDER lib
                                                 ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/lib)

set( SSE ${BASE_INC_FILES} eveye_pinter.c eveye_sad.c)

if( UNIX OR MINGW )
  set_property( SOURCE ${SSE} APPEND PROPERTY COMPILE_FLAGS "-msse4.2" )
endif()

# decoder library
set( DEC_LIB_NAME eveyd )

file (GLOB DEC_LIB_FUNCTION_SRC "eveyd*.c")
file (GLOB DEC_LIB_FUNCTION_INC "eveyd*.h")

add_library( ${DEC_LIB_NAME} STATIC ${DEC_LIB_FUNCTION_SRC} ${DEC_LIB_FUNCTION_INC} )

target_compile_definitions( ${DEC_LIB_NAME} PUBLIC )
target_include_directories( ${DEC_LIB_NAME} PUBLIC . .. )
target_link_libraries( ${DEC_LIB_NAME} evey)
if( UNIX OR MINGW )  
  target_link_libraries(${DEC_LIB_NAME} m)
endif()

set_target_properties(${DEC_LIB_NAME} PROPERTIES FOLDER lib
                                                 ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/lib)