cmake_minimum_required(VERSION 3.7)
project(sockets)

set(CMAKE_CXX_STANDARD 11)

include_directories(${CMAKE_SOURCE_DIR}/src/sockets/include)
set(SOURCE_FILES src/main.cpp src/Echo.h src/Echo.cpp)

add_executable(server ${SOURCE_FILES})
target_link_libraries(server sockets)
