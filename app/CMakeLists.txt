include_directories ("${PROJECT_SOURCE_DIR}/app")

# Adds sources to the Solution Explorer
add_executable (eveya_encoder eveya_encoder.c eveya_util.h eveya_args.h)
add_executable (eveya_decoder eveya_decoder.c eveya_util.h eveya_args.h)
add_executable (eveya_bitstream_merge eveya_bitstream_merge.c eveya_util.h eveya_args.h)
add_executable (echo_server echo_server.c)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Properties->Linker->Input->Additional Dependencies
target_link_libraries (eveya_encoder eveye)
target_link_libraries (eveya_decoder eveyd)
target_link_libraries (eveya_bitstream_merge eveye)
target_link_libraries (eveya_bitstream_merge eveyd)

# Creates a folder "executables" and adds target 
# project (app.vcproj) under it
set_property(TARGET eveya_encoder PROPERTY FOLDER "app")
set_property(TARGET eveya_decoder PROPERTY FOLDER "app")
set_property(TARGET eveya_bitstream_merge PROPERTY FOLDER "app")
set_property(TARGET echo_server PROPERTY FOLDER "app")

set_target_properties(eveya_encoder PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(eveya_decoder PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(eveya_bitstream_merge PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(echo_server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
                      
target_include_directories( eveya_encoder PUBLIC . .. ../inc/ ../src/dec ../src/enc)
target_include_directories( eveya_decoder PUBLIC . .. ../inc/ ../src/dec ../src/enc)
target_include_directories( eveya_bitstream_merge PUBLIC . .. ../inc/ ../src/dec ../src/enc)

if( MSVC )                      
    target_compile_definitions( eveya_encoder PUBLIC _CRT_SECURE_NO_WARNINGS )
    target_compile_definitions( eveya_decoder PUBLIC _CRT_SECURE_NO_WARNINGS )
    target_compile_definitions( eveya_bitstream_merge PUBLIC _CRT_SECURE_NO_WARNINGS )
    target_compile_definitions( echo_server PUBLIC _CRT_SECURE_NO_WARNINGS )
endif()

