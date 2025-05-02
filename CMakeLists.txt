cmake_minimum_required(VERSION 2.8)
project(server)

include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-unused-but-set-variable")

include_directories(.)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

set(LIB_SRC
    server/log.cpp
    server/util.cpp
    server/config.cpp
    server/thread.cpp
    server/fiber.cpp
    server/scheduler.cpp
    server/iomanager.cpp
    server/timer.cpp
    server/fd_manager.cpp
    server/hook.cpp
    )

add_library(server SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(server)
#add_library(server_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (server_static PROPERTIES OUTPUT_NAME "server")

find_library(YAMLCPP yaml-cpp)

set(LIB_LIB
    server
    dl
    pthread
    ${YAMLCPP}
    )

message("***", ${LIB_LIB})

add_executable(test tests/test.cpp)
add_dependencies(test server)
force_redefine_file_macro_for_sources(test)
target_link_libraries(test ${LIB_LIB})

add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config server)
force_redefine_file_macro_for_sources(test_config)
target_link_libraries(test_config ${LIB_LIB})

add_executable(test_thread tests/test_thread.cpp)
add_dependencies(test_thread server)
force_redefine_file_macro_for_sources(test_thread)
target_link_libraries(test_thread ${LIB_LIB})

add_executable(test_util tests/test_util.cpp)
add_dependencies(test_util server)
force_redefine_file_macro_for_sources(test_util)
target_link_libraries(test_util ${LIB_LIB})

add_executable(test_fiber tests/test_fiber.cpp)
add_dependencies(test_fiber server)
force_redefine_file_macro_for_sources(test_fiber)
target_link_libraries(test_fiber ${LIB_LIB})

add_executable(test_scheduler tests/test_scheduler.cpp)
add_dependencies(test_scheduler server)
force_redefine_file_macro_for_sources(test_scheduler)
target_link_libraries(test_scheduler ${LIB_LIB})

add_executable(test_iomanager tests/test_iomanager.cpp)
add_dependencies(test_iomanager server)
force_redefine_file_macro_for_sources(test_iomanager)
target_link_libraries(test_iomanager ${LIB_LIB})

add_executable(test_hook tests/test_hook.cpp)
add_dependencies(test_hook server)
force_redefine_file_macro_for_sources(test_hook)
target_link_libraries(test_hook ${LIB_LIB})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)