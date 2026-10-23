cmake_minimum_required(VERSION 3.16)
project(cpp_task_manager VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TASKMGR_BUILD_TESTS "Build unit tests" ON)

add_library(taskmgr_lib STATIC
    src/Task.cpp
    src/Storage.cpp
    src/TaskManager.cpp
    src/Cli.cpp
)
target_include_directories(taskmgr_lib PUBLIC include)

if(MSVC)
    target_compile_options(taskmgr_lib PUBLIC /W4 /utf-8)
else()
    target_compile_options(taskmgr_lib PUBLIC -Wall -Wextra -Wpedantic)
endif()

add_executable(taskmgr src/main.cpp)
target_link_libraries(taskmgr PRIVATE taskmgr_lib)

if(TASKMGR_BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz)
    FetchContent_MakeAvailable(googletest)

    add_executable(taskmgr_tests
        tests/test_task.cpp
        tests/test_storage.cpp
        tests/test_manager.cpp
        tests/test_cli.cpp
    )
    target_link_libraries(taskmgr_tests PRIVATE taskmgr_lib GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(taskmgr_tests)
endif()
