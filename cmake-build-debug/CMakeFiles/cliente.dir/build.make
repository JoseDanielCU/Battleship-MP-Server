# CMAKE generated file: DO NOT EDIT!
# Generated by "MinGW Makefiles" Generator, CMake Version 3.30

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

SHELL = cmd.exe

# The CMake executable.
CMAKE_COMMAND = C:\Users\MLGda\AppData\Local\Programs\CLion\bin\cmake\win\x64\bin\cmake.exe

# The command to remove a file.
RM = C:\Users\MLGda\AppData\Local\Programs\CLion\bin\cmake\win\x64\bin\cmake.exe -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = C:\Projects\Battleship-MP-Server

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = C:\Projects\Battleship-MP-Server\cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/cliente.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/cliente.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/cliente.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/cliente.dir/flags.make

CMakeFiles/cliente.dir/client.cpp.obj: CMakeFiles/cliente.dir/flags.make
CMakeFiles/cliente.dir/client.cpp.obj: C:/Projects/Battleship-MP-Server/client.cpp
CMakeFiles/cliente.dir/client.cpp.obj: CMakeFiles/cliente.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=C:\Projects\Battleship-MP-Server\cmake-build-debug\CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/cliente.dir/client.cpp.obj"
	C:\Users\MLGda\AppData\Local\Programs\CLion\bin\mingw\bin\g++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/cliente.dir/client.cpp.obj -MF CMakeFiles\cliente.dir\client.cpp.obj.d -o CMakeFiles\cliente.dir\client.cpp.obj -c C:\Projects\Battleship-MP-Server\client.cpp

CMakeFiles/cliente.dir/client.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/cliente.dir/client.cpp.i"
	C:\Users\MLGda\AppData\Local\Programs\CLion\bin\mingw\bin\g++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E C:\Projects\Battleship-MP-Server\client.cpp > CMakeFiles\cliente.dir\client.cpp.i

CMakeFiles/cliente.dir/client.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/cliente.dir/client.cpp.s"
	C:\Users\MLGda\AppData\Local\Programs\CLion\bin\mingw\bin\g++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S C:\Projects\Battleship-MP-Server\client.cpp -o CMakeFiles\cliente.dir\client.cpp.s

# Object files for target cliente
cliente_OBJECTS = \
"CMakeFiles/cliente.dir/client.cpp.obj"

# External object files for target cliente
cliente_EXTERNAL_OBJECTS =

cliente.exe: CMakeFiles/cliente.dir/client.cpp.obj
cliente.exe: CMakeFiles/cliente.dir/build.make
cliente.exe: CMakeFiles/cliente.dir/linkLibs.rsp
cliente.exe: CMakeFiles/cliente.dir/objects1.rsp
cliente.exe: CMakeFiles/cliente.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=C:\Projects\Battleship-MP-Server\cmake-build-debug\CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable cliente.exe"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles\cliente.dir\link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/cliente.dir/build: cliente.exe
.PHONY : CMakeFiles/cliente.dir/build

CMakeFiles/cliente.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles\cliente.dir\cmake_clean.cmake
.PHONY : CMakeFiles/cliente.dir/clean

CMakeFiles/cliente.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MinGW Makefiles" C:\Projects\Battleship-MP-Server C:\Projects\Battleship-MP-Server C:\Projects\Battleship-MP-Server\cmake-build-debug C:\Projects\Battleship-MP-Server\cmake-build-debug C:\Projects\Battleship-MP-Server\cmake-build-debug\CMakeFiles\cliente.dir\DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/cliente.dir/depend

