cmake_minimum_required(VERSION 3.7)
project(sockets)

set(CMAKE_CXX_STANDARD 11)

include_directories(${CMAKE_SOURCE_DIR}/src/sockets/include)
message(${CMAKE_SOURCE_DIR})
set(SOURCE_FILES src/main.cpp src/Echo.cpp src/Echo.h)

add_executable(client ${SOURCE_FILES})
target_link_libraries(client sockets)
