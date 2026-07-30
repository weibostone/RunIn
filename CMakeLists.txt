cmake_minimum_required(VERSION 3.15)
project(RunIn CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_compile_definitions(UNICODE _UNICODE)
add_compile_options(/utf-8)
add_executable(ri WIN32 main.cpp)
set_target_properties(ri PROPERTIES WIN32_EXECUTABLE TRUE)
