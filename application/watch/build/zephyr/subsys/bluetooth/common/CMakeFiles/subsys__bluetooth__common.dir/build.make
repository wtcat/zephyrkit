# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
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
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/wt/zephyr-kit/project_wkpace

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/wt/zephyr-kit/project_wkpace/build

# Include any dependencies generated for this target.
include zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/depend.make

# Include the progress variables for this target.
include zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/progress.make

# Include the compile flags for this target's objects.
include zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/flags.make

zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/dummy.c.obj: zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/flags.make
zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/dummy.c.obj: /home/wt/zephyr-kit/zephyr_pkg/zephyr/subsys/bluetooth/common/dummy.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/wt/zephyr-kit/project_wkpace/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/dummy.c.obj"
	cd /home/wt/zephyr-kit/project_wkpace/build/zephyr/subsys/bluetooth/common && ccache /home/wt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/subsys__bluetooth__common.dir/dummy.c.obj   -c /home/wt/zephyr-kit/zephyr_pkg/zephyr/subsys/bluetooth/common/dummy.c

zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/dummy.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/subsys__bluetooth__common.dir/dummy.c.i"
	cd /home/wt/zephyr-kit/project_wkpace/build/zephyr/subsys/bluetooth/common && /home/wt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/wt/zephyr-kit/zephyr_pkg/zephyr/subsys/bluetooth/common/dummy.c > CMakeFiles/subsys__bluetooth__common.dir/dummy.c.i

zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/dummy.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/subsys__bluetooth__common.dir/dummy.c.s"
	cd /home/wt/zephyr-kit/project_wkpace/build/zephyr/subsys/bluetooth/common && /home/wt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/wt/zephyr-kit/zephyr_pkg/zephyr/subsys/bluetooth/common/dummy.c -o CMakeFiles/subsys__bluetooth__common.dir/dummy.c.s

zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/log.c.obj: zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/flags.make
zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/log.c.obj: /home/wt/zephyr-kit/zephyr_pkg/zephyr/subsys/bluetooth/common/log.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/wt/zephyr-kit/project_wkpace/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/log.c.obj"
	cd /home/wt/zephyr-kit/project_wkpace/build/zephyr/subsys/bluetooth/common && ccache /home/wt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/subsys__bluetooth__common.dir/log.c.obj   -c /home/wt/zephyr-kit/zephyr_pkg/zephyr/subsys/bluetooth/common/log.c

zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/log.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/subsys__bluetooth__common.dir/log.c.i"
	cd /home/wt/zephyr-kit/project_wkpace/build/zephyr/subsys/bluetooth/common && /home/wt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/wt/zephyr-kit/zephyr_pkg/zephyr/subsys/bluetooth/common/log.c > CMakeFiles/subsys__bluetooth__common.dir/log.c.i

zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/log.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/subsys__bluetooth__common.dir/log.c.s"
	cd /home/wt/zephyr-kit/project_wkpace/build/zephyr/subsys/bluetooth/common && /home/wt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/wt/zephyr-kit/zephyr_pkg/zephyr/subsys/bluetooth/common/log.c -o CMakeFiles/subsys__bluetooth__common.dir/log.c.s

zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/rpa.c.obj: zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/flags.make
zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/rpa.c.obj: /home/wt/zephyr-kit/zephyr_pkg/zephyr/subsys/bluetooth/common/rpa.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/wt/zephyr-kit/project_wkpace/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/rpa.c.obj"
	cd /home/wt/zephyr-kit/project_wkpace/build/zephyr/subsys/bluetooth/common && ccache /home/wt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/subsys__bluetooth__common.dir/rpa.c.obj   -c /home/wt/zephyr-kit/zephyr_pkg/zephyr/subsys/bluetooth/common/rpa.c

zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/rpa.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/subsys__bluetooth__common.dir/rpa.c.i"
	cd /home/wt/zephyr-kit/project_wkpace/build/zephyr/subsys/bluetooth/common && /home/wt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/wt/zephyr-kit/zephyr_pkg/zephyr/subsys/bluetooth/common/rpa.c > CMakeFiles/subsys__bluetooth__common.dir/rpa.c.i

zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/rpa.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/subsys__bluetooth__common.dir/rpa.c.s"
	cd /home/wt/zephyr-kit/project_wkpace/build/zephyr/subsys/bluetooth/common && /home/wt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/wt/zephyr-kit/zephyr_pkg/zephyr/subsys/bluetooth/common/rpa.c -o CMakeFiles/subsys__bluetooth__common.dir/rpa.c.s

# Object files for target subsys__bluetooth__common
subsys__bluetooth__common_OBJECTS = \
"CMakeFiles/subsys__bluetooth__common.dir/dummy.c.obj" \
"CMakeFiles/subsys__bluetooth__common.dir/log.c.obj" \
"CMakeFiles/subsys__bluetooth__common.dir/rpa.c.obj"

# External object files for target subsys__bluetooth__common
subsys__bluetooth__common_EXTERNAL_OBJECTS =

zephyr/subsys/bluetooth/common/libsubsys__bluetooth__common.a: zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/dummy.c.obj
zephyr/subsys/bluetooth/common/libsubsys__bluetooth__common.a: zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/log.c.obj
zephyr/subsys/bluetooth/common/libsubsys__bluetooth__common.a: zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/rpa.c.obj
zephyr/subsys/bluetooth/common/libsubsys__bluetooth__common.a: zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/build.make
zephyr/subsys/bluetooth/common/libsubsys__bluetooth__common.a: zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/wt/zephyr-kit/project_wkpace/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking C static library libsubsys__bluetooth__common.a"
	cd /home/wt/zephyr-kit/project_wkpace/build/zephyr/subsys/bluetooth/common && $(CMAKE_COMMAND) -P CMakeFiles/subsys__bluetooth__common.dir/cmake_clean_target.cmake
	cd /home/wt/zephyr-kit/project_wkpace/build/zephyr/subsys/bluetooth/common && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/subsys__bluetooth__common.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/build: zephyr/subsys/bluetooth/common/libsubsys__bluetooth__common.a

.PHONY : zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/build

zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/clean:
	cd /home/wt/zephyr-kit/project_wkpace/build/zephyr/subsys/bluetooth/common && $(CMAKE_COMMAND) -P CMakeFiles/subsys__bluetooth__common.dir/cmake_clean.cmake
.PHONY : zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/clean

zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/depend:
	cd /home/wt/zephyr-kit/project_wkpace/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/wt/zephyr-kit/project_wkpace /home/wt/zephyr-kit/zephyr_pkg/zephyr/subsys/bluetooth/common /home/wt/zephyr-kit/project_wkpace/build /home/wt/zephyr-kit/project_wkpace/build/zephyr/subsys/bluetooth/common /home/wt/zephyr-kit/project_wkpace/build/zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : zephyr/subsys/bluetooth/common/CMakeFiles/subsys__bluetooth__common.dir/depend

