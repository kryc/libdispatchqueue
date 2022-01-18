cmake_minimum_required(VERSION 3.10)

# set the project name
project(libDispatchQueue VERSION 0.1)

# specify clang
SET(CMAKE_C_COMPILER clang-12)
SET(CMAKE_CXX_COMPILER clang++-12)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set compiler flags
set(STD_COMPILE_FLAGS "-Wall -Werror -fno-exceptions")
set(STD_LINKER_FLAGS "-lpthread")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STD_COMPILE_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${STD_COMPILE_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${STD_LINKER_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -fno-omit-frame-pointer -fsanitize=address -DDEBUG")

# add the library
file(GLOB LIBSOURCES "./src/*.cpp")
add_library(dispatchqueue ${LIBSOURCES})
target_include_directories(dispatchqueue PUBLIC "./include/")

# add the test
file(GLOB TESTSOURCES "./tests/*.cpp")
add_executable(test ${TESTSOURCES})
target_link_libraries(test dispatchqueue)
target_include_directories(test PUBLIC "./include/")

configure_file(Config.h.in Config.h)
