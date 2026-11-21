cmake_minimum_required(VERSION 3.10)
project(DdareungiNetwork CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRCS src/*.cpp src/modules/*.cpp)
add_executable(ddareungi ${SRCS})
target_include_directories(ddareungi PRIVATE include)

# 소스가 UTF-8(한글 문자열·주석) → MSVC 는 기본이 CP949 라 /utf-8 이 없으면 빌드 오류
if(MSVC)
    target_compile_options(ddareungi PRIVATE /utf-8)
endif()
