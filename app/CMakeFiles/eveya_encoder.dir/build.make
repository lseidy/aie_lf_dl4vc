# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc

# Include any dependencies generated for this target.
include app/CMakeFiles/eveya_encoder.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include app/CMakeFiles/eveya_encoder.dir/compiler_depend.make

# Include the progress variables for this target.
include app/CMakeFiles/eveya_encoder.dir/progress.make

# Include the compile flags for this target's objects.
include app/CMakeFiles/eveya_encoder.dir/flags.make

app/CMakeFiles/eveya_encoder.dir/eveya_encoder.c.o: app/CMakeFiles/eveya_encoder.dir/flags.make
app/CMakeFiles/eveya_encoder.dir/eveya_encoder.c.o: app/eveya_encoder.c
app/CMakeFiles/eveya_encoder.dir/eveya_encoder.c.o: app/CMakeFiles/eveya_encoder.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object app/CMakeFiles/eveya_encoder.dir/eveya_encoder.c.o"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/app && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT app/CMakeFiles/eveya_encoder.dir/eveya_encoder.c.o -MF CMakeFiles/eveya_encoder.dir/eveya_encoder.c.o.d -o CMakeFiles/eveya_encoder.dir/eveya_encoder.c.o -c /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/app/eveya_encoder.c

app/CMakeFiles/eveya_encoder.dir/eveya_encoder.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/eveya_encoder.dir/eveya_encoder.c.i"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/app && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/app/eveya_encoder.c > CMakeFiles/eveya_encoder.dir/eveya_encoder.c.i

app/CMakeFiles/eveya_encoder.dir/eveya_encoder.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/eveya_encoder.dir/eveya_encoder.c.s"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/app && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/app/eveya_encoder.c -o CMakeFiles/eveya_encoder.dir/eveya_encoder.c.s

# Object files for target eveya_encoder
eveya_encoder_OBJECTS = \
"CMakeFiles/eveya_encoder.dir/eveya_encoder.c.o"

# External object files for target eveya_encoder
eveya_encoder_EXTERNAL_OBJECTS =

bin/eveya_encoder: app/CMakeFiles/eveya_encoder.dir/eveya_encoder.c.o
bin/eveya_encoder: app/CMakeFiles/eveya_encoder.dir/build.make
bin/eveya_encoder: lib/libeveye.a
bin/eveya_encoder: lib/libevey.a
bin/eveya_encoder: app/CMakeFiles/eveya_encoder.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable ../bin/eveya_encoder"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/app && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/eveya_encoder.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
app/CMakeFiles/eveya_encoder.dir/build: bin/eveya_encoder
.PHONY : app/CMakeFiles/eveya_encoder.dir/build

app/CMakeFiles/eveya_encoder.dir/clean:
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/app && $(CMAKE_COMMAND) -P CMakeFiles/eveya_encoder.dir/cmake_clean.cmake
.PHONY : app/CMakeFiles/eveya_encoder.dir/clean

app/CMakeFiles/eveya_encoder.dir/depend:
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/app /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/app /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/app/CMakeFiles/eveya_encoder.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : app/CMakeFiles/eveya_encoder.dir/depend
