cmake_minimum_required(VERSION 3.13)
project(http-server-starter-cpp)
set(CMAKE_CXX_STANDARD 20) # Enable the C++20 standard

add_subdirectory(src)

add_executable(
  server
  src/server.cpp
)
target_include_directories(server PRIVATE include)
target_link_libraries(server ${PROJECT_NAME})
