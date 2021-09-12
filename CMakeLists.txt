cmake_minimum_required(VERSION 3.18)
project(MCGA_proc)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

option(MCGA_proc_tests "Build MCGA Proc tests" OFF)

add_library(mcga_proc INTERFACE)
target_include_directories(mcga_proc INTERFACE include)

install(DIRECTORY include DESTINATION .)

if (MCGA_proc_tests)
    add_executable(mcga_proc_test
            tests/message_test.cpp
            tests/pipe_test.cpp
            tests/subprocess_test.cpp
            tests/worker_subprocess_test.cpp
            )
    target_link_libraries(mcga_proc_test mcga_test mcga_proc)
endif ()
