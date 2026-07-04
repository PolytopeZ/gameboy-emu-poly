cmake_minimum_required(VERSION 3.15)
project(gbemu CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(gbemu src/main.cpp src/sm83.cpp)
target_include_directories(gbemu PRIVATE src)
target_compile_options(gbemu PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)
