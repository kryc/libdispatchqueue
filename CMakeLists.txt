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
target_include_directories(dispatchqueue
                            PUBLIC
                                ./include/
                            PRIVATE
                                ./src/
                        )

# add the test
add_custom_target(tests)
file(GLOB TESTS "tests/*.cpp")
foreach(TEST ${TESTS})
    get_filename_component(TESTNAME ${TEST} NAME_WE)
    add_executable(${TESTNAME} EXCLUDE_FROM_ALL ${TEST} ${SOURCES})
    target_include_directories(${TESTNAME}
                                PUBLIC
                                    ./include/)
    target_link_libraries(${TESTNAME} dispatchqueue)
    add_dependencies(tests ${TESTNAME})
endforeach()

configure_file(Config.h.in Config.h)
