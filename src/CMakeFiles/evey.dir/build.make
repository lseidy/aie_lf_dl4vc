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
include src/CMakeFiles/evey.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include src/CMakeFiles/evey.dir/compiler_depend.make

# Include the progress variables for this target.
include src/CMakeFiles/evey.dir/progress.make

# Include the compile flags for this target's objects.
include src/CMakeFiles/evey.dir/flags.make

src/CMakeFiles/evey.dir/evey.c.o: src/CMakeFiles/evey.dir/flags.make
src/CMakeFiles/evey.dir/evey.c.o: src/evey.c
src/CMakeFiles/evey.dir/evey.c.o: src/CMakeFiles/evey.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object src/CMakeFiles/evey.dir/evey.c.o"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/CMakeFiles/evey.dir/evey.c.o -MF CMakeFiles/evey.dir/evey.c.o.d -o CMakeFiles/evey.dir/evey.c.o -c /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey.c

src/CMakeFiles/evey.dir/evey.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/evey.dir/evey.c.i"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey.c > CMakeFiles/evey.dir/evey.c.i

src/CMakeFiles/evey.dir/evey.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/evey.dir/evey.c.s"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey.c -o CMakeFiles/evey.dir/evey.c.s

src/CMakeFiles/evey.dir/evey_inter.c.o: src/CMakeFiles/evey.dir/flags.make
src/CMakeFiles/evey.dir/evey_inter.c.o: src/evey_inter.c
src/CMakeFiles/evey.dir/evey_inter.c.o: src/CMakeFiles/evey.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object src/CMakeFiles/evey.dir/evey_inter.c.o"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -msse4.1 -MD -MT src/CMakeFiles/evey.dir/evey_inter.c.o -MF CMakeFiles/evey.dir/evey_inter.c.o.d -o CMakeFiles/evey.dir/evey_inter.c.o -c /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_inter.c

src/CMakeFiles/evey.dir/evey_inter.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/evey.dir/evey_inter.c.i"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -msse4.1 -E /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_inter.c > CMakeFiles/evey.dir/evey_inter.c.i

src/CMakeFiles/evey.dir/evey_inter.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/evey.dir/evey_inter.c.s"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -msse4.1 -S /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_inter.c -o CMakeFiles/evey.dir/evey_inter.c.s

src/CMakeFiles/evey.dir/evey_intra.c.o: src/CMakeFiles/evey.dir/flags.make
src/CMakeFiles/evey.dir/evey_intra.c.o: src/evey_intra.c
src/CMakeFiles/evey.dir/evey_intra.c.o: src/CMakeFiles/evey.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object src/CMakeFiles/evey.dir/evey_intra.c.o"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/CMakeFiles/evey.dir/evey_intra.c.o -MF CMakeFiles/evey.dir/evey_intra.c.o.d -o CMakeFiles/evey.dir/evey_intra.c.o -c /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_intra.c

src/CMakeFiles/evey.dir/evey_intra.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/evey.dir/evey_intra.c.i"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_intra.c > CMakeFiles/evey.dir/evey_intra.c.i

src/CMakeFiles/evey.dir/evey_intra.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/evey.dir/evey_intra.c.s"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_intra.c -o CMakeFiles/evey.dir/evey_intra.c.s

src/CMakeFiles/evey.dir/evey_itdq.c.o: src/CMakeFiles/evey.dir/flags.make
src/CMakeFiles/evey.dir/evey_itdq.c.o: src/evey_itdq.c
src/CMakeFiles/evey.dir/evey_itdq.c.o: src/CMakeFiles/evey.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object src/CMakeFiles/evey.dir/evey_itdq.c.o"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -msse4.1 -MD -MT src/CMakeFiles/evey.dir/evey_itdq.c.o -MF CMakeFiles/evey.dir/evey_itdq.c.o.d -o CMakeFiles/evey.dir/evey_itdq.c.o -c /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_itdq.c

src/CMakeFiles/evey.dir/evey_itdq.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/evey.dir/evey_itdq.c.i"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -msse4.1 -E /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_itdq.c > CMakeFiles/evey.dir/evey_itdq.c.i

src/CMakeFiles/evey.dir/evey_itdq.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/evey.dir/evey_itdq.c.s"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -msse4.1 -S /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_itdq.c -o CMakeFiles/evey.dir/evey_itdq.c.s

src/CMakeFiles/evey.dir/evey_lf.c.o: src/CMakeFiles/evey.dir/flags.make
src/CMakeFiles/evey.dir/evey_lf.c.o: src/evey_lf.c
src/CMakeFiles/evey.dir/evey_lf.c.o: src/CMakeFiles/evey.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object src/CMakeFiles/evey.dir/evey_lf.c.o"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/CMakeFiles/evey.dir/evey_lf.c.o -MF CMakeFiles/evey.dir/evey_lf.c.o.d -o CMakeFiles/evey.dir/evey_lf.c.o -c /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_lf.c

src/CMakeFiles/evey.dir/evey_lf.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/evey.dir/evey_lf.c.i"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_lf.c > CMakeFiles/evey.dir/evey_lf.c.i

src/CMakeFiles/evey.dir/evey_lf.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/evey.dir/evey_lf.c.s"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_lf.c -o CMakeFiles/evey.dir/evey_lf.c.s

src/CMakeFiles/evey.dir/evey_picman.c.o: src/CMakeFiles/evey.dir/flags.make
src/CMakeFiles/evey.dir/evey_picman.c.o: src/evey_picman.c
src/CMakeFiles/evey.dir/evey_picman.c.o: src/CMakeFiles/evey.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object src/CMakeFiles/evey.dir/evey_picman.c.o"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/CMakeFiles/evey.dir/evey_picman.c.o -MF CMakeFiles/evey.dir/evey_picman.c.o.d -o CMakeFiles/evey.dir/evey_picman.c.o -c /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_picman.c

src/CMakeFiles/evey.dir/evey_picman.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/evey.dir/evey_picman.c.i"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_picman.c > CMakeFiles/evey.dir/evey_picman.c.i

src/CMakeFiles/evey.dir/evey_picman.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/evey.dir/evey_picman.c.s"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_picman.c -o CMakeFiles/evey.dir/evey_picman.c.s

src/CMakeFiles/evey.dir/evey_port.c.o: src/CMakeFiles/evey.dir/flags.make
src/CMakeFiles/evey.dir/evey_port.c.o: src/evey_port.c
src/CMakeFiles/evey.dir/evey_port.c.o: src/CMakeFiles/evey.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building C object src/CMakeFiles/evey.dir/evey_port.c.o"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/CMakeFiles/evey.dir/evey_port.c.o -MF CMakeFiles/evey.dir/evey_port.c.o.d -o CMakeFiles/evey.dir/evey_port.c.o -c /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_port.c

src/CMakeFiles/evey.dir/evey_port.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/evey.dir/evey_port.c.i"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_port.c > CMakeFiles/evey.dir/evey_port.c.i

src/CMakeFiles/evey.dir/evey_port.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/evey.dir/evey_port.c.s"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_port.c -o CMakeFiles/evey.dir/evey_port.c.s

src/CMakeFiles/evey.dir/evey_recon.c.o: src/CMakeFiles/evey.dir/flags.make
src/CMakeFiles/evey.dir/evey_recon.c.o: src/evey_recon.c
src/CMakeFiles/evey.dir/evey_recon.c.o: src/CMakeFiles/evey.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building C object src/CMakeFiles/evey.dir/evey_recon.c.o"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/CMakeFiles/evey.dir/evey_recon.c.o -MF CMakeFiles/evey.dir/evey_recon.c.o.d -o CMakeFiles/evey.dir/evey_recon.c.o -c /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_recon.c

src/CMakeFiles/evey.dir/evey_recon.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/evey.dir/evey_recon.c.i"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_recon.c > CMakeFiles/evey.dir/evey_recon.c.i

src/CMakeFiles/evey.dir/evey_recon.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/evey.dir/evey_recon.c.s"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_recon.c -o CMakeFiles/evey.dir/evey_recon.c.s

src/CMakeFiles/evey.dir/evey_tbl.c.o: src/CMakeFiles/evey.dir/flags.make
src/CMakeFiles/evey.dir/evey_tbl.c.o: src/evey_tbl.c
src/CMakeFiles/evey.dir/evey_tbl.c.o: src/CMakeFiles/evey.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building C object src/CMakeFiles/evey.dir/evey_tbl.c.o"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/CMakeFiles/evey.dir/evey_tbl.c.o -MF CMakeFiles/evey.dir/evey_tbl.c.o.d -o CMakeFiles/evey.dir/evey_tbl.c.o -c /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_tbl.c

src/CMakeFiles/evey.dir/evey_tbl.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/evey.dir/evey_tbl.c.i"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_tbl.c > CMakeFiles/evey.dir/evey_tbl.c.i

src/CMakeFiles/evey.dir/evey_tbl.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/evey.dir/evey_tbl.c.s"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_tbl.c -o CMakeFiles/evey.dir/evey_tbl.c.s

src/CMakeFiles/evey.dir/evey_util.c.o: src/CMakeFiles/evey.dir/flags.make
src/CMakeFiles/evey.dir/evey_util.c.o: src/evey_util.c
src/CMakeFiles/evey.dir/evey_util.c.o: src/CMakeFiles/evey.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Building C object src/CMakeFiles/evey.dir/evey_util.c.o"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -msse4.1 -MD -MT src/CMakeFiles/evey.dir/evey_util.c.o -MF CMakeFiles/evey.dir/evey_util.c.o.d -o CMakeFiles/evey.dir/evey_util.c.o -c /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_util.c

src/CMakeFiles/evey.dir/evey_util.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/evey.dir/evey_util.c.i"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -msse4.1 -E /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_util.c > CMakeFiles/evey.dir/evey_util.c.i

src/CMakeFiles/evey.dir/evey_util.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/evey.dir/evey_util.c.s"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -msse4.1 -S /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/evey_util.c -o CMakeFiles/evey.dir/evey_util.c.s

# Object files for target evey
evey_OBJECTS = \
"CMakeFiles/evey.dir/evey.c.o" \
"CMakeFiles/evey.dir/evey_inter.c.o" \
"CMakeFiles/evey.dir/evey_intra.c.o" \
"CMakeFiles/evey.dir/evey_itdq.c.o" \
"CMakeFiles/evey.dir/evey_lf.c.o" \
"CMakeFiles/evey.dir/evey_picman.c.o" \
"CMakeFiles/evey.dir/evey_port.c.o" \
"CMakeFiles/evey.dir/evey_recon.c.o" \
"CMakeFiles/evey.dir/evey_tbl.c.o" \
"CMakeFiles/evey.dir/evey_util.c.o"

# External object files for target evey
evey_EXTERNAL_OBJECTS =

lib/libevey.a: src/CMakeFiles/evey.dir/evey.c.o
lib/libevey.a: src/CMakeFiles/evey.dir/evey_inter.c.o
lib/libevey.a: src/CMakeFiles/evey.dir/evey_intra.c.o
lib/libevey.a: src/CMakeFiles/evey.dir/evey_itdq.c.o
lib/libevey.a: src/CMakeFiles/evey.dir/evey_lf.c.o
lib/libevey.a: src/CMakeFiles/evey.dir/evey_picman.c.o
lib/libevey.a: src/CMakeFiles/evey.dir/evey_port.c.o
lib/libevey.a: src/CMakeFiles/evey.dir/evey_recon.c.o
lib/libevey.a: src/CMakeFiles/evey.dir/evey_tbl.c.o
lib/libevey.a: src/CMakeFiles/evey.dir/evey_util.c.o
lib/libevey.a: src/CMakeFiles/evey.dir/build.make
lib/libevey.a: src/CMakeFiles/evey.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/CMakeFiles --progress-num=$(CMAKE_PROGRESS_11) "Linking C static library ../lib/libevey.a"
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && $(CMAKE_COMMAND) -P CMakeFiles/evey.dir/cmake_clean_target.cmake
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/evey.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/CMakeFiles/evey.dir/build: lib/libevey.a
.PHONY : src/CMakeFiles/evey.dir/build

src/CMakeFiles/evey.dir/clean:
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src && $(CMAKE_COMMAND) -P CMakeFiles/evey.dir/cmake_clean.cmake
.PHONY : src/CMakeFiles/evey.dir/clean

src/CMakeFiles/evey.dir/depend:
	cd /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src /mnt/c/Users/lucas/Documents/TCC/aie_lf_dl4vc/src/CMakeFiles/evey.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/CMakeFiles/evey.dir/depend

