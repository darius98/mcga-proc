cmake_minimum_required(VERSION 3.12.1)
project(MCGA_proc)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

option(MCGA_proc_tests "Build MCGA Proc tests" OFF)

list(APPEND MCGA_PROC_SRC
        src/message.cpp
        src/pipe.cpp
        src/worker_subprocess.cpp
        )
if (UNIX OR APPLE)
    list(APPEND MCGA_PROC_SRC
            src/pipe_posix.cpp
            src/subprocess_posix.cpp
            )
endif ()
add_library(mcga_proc STATIC ${MCGA_PROC_SRC})
target_include_directories(mcga_proc PUBLIC include)

install(DIRECTORY include DESTINATION .)

if (MCGA_proc_tests)
    add_executable(mcga_proc_test
            tests/message_test.cpp
            tests/pipe_test.cpp
            tests/subprocess_test.cpp
            tests/worker_subprocess_test.cpp
            )
    target_link_libraries(mcga_proc_test kktest mcga_proc)
endif ()
