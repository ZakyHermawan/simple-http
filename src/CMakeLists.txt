cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20) # Enable the C++20 standard

add_library(${PROJECT_NAME} request.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include)
