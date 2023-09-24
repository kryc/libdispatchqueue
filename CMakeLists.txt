cmake_minimum_required(VERSION 3.10)

# set the project name
project(libDispatchQueue VERSION 0.1)

# specify clang
SET(CMAKE_C_COMPILER clang)
SET(CMAKE_CXX_COMPILER clang++)

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
add_custom_target(libdispatchqueue_tests)
file(GLOB TESTS "tests/*.cpp")
foreach(TEST ${TESTS})
    get_filename_component(TESTNAME ${TEST} NAME_WE)
    add_executable(${TESTNAME} EXCLUDE_FROM_ALL ${TEST} ${SOURCES})
    target_compile_definitions(${TESTNAME} PUBLIC TEST)
    target_include_directories(${TESTNAME}
                                PUBLIC
                                    ./include/)
    target_link_libraries(${TESTNAME} dispatchqueue)
    add_dependencies(libdispatchqueue_tests ${TESTNAME})
endforeach()

add_custom_target(libdispatchqueue_fuzzers)
set(FUZZER_SOURCES ${SOURCES})
file(GLOB FUZZERS "fuzzers/*.cpp")
foreach(FUZZER ${FUZZERS})
    get_filename_component(FUZZERNAME ${FUZZER} NAME_WE)
    add_executable(${FUZZERNAME} ${FUZZER} ${FUZZER_SOURCES})
    target_compile_definitions(${TESTNAME} PUBLIC FUZZER)
    target_compile_options(${FUZZERNAME} PUBLIC -fsanitize=fuzzer,address)
    target_link_options(${FUZZERNAME} PUBLIC -fsanitize=fuzzer,address)
    target_include_directories(${FUZZERNAME}
                                PUBLIC
                                    ./include/
                            )
    target_link_libraries(${FUZZERNAME} dispatchqueue)
    add_dependencies(libdispatchqueue_fuzzers ${FUZZERNAME})
endforeach()

configure_file(Config.h.in Config.h)
