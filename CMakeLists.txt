cmake_minimum_required(VERSION 3.10)
project(VertexProcessor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(vertex_processor src/main.cpp)

if(MSVC)
    target_compile_options(vertex_processor PRIVATE /W4 /O2)
else()
    target_compile_options(vertex_processor PRIVATE -Wall -Wextra -O3)
endif()
