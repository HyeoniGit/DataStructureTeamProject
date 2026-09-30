cmake_minimum_required(VERSION 3.10)
project(DdareungiNetwork CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRCS src/*.cpp src/modules/*.cpp)
add_executable(ddareungi ${SRCS})
target_include_directories(ddareungi PRIVATE include)
