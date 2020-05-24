cmake_minimum_required(VERSION 3.5)

project(aviete-cec LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(aviete-cec main.cpp)
target_link_libraries(aviete-cec cec dl xdo)

