cmake_minimum_required(VERSION 3.7)
project(sockets)

set(CMAKE_CXX_STANDARD 11)

include_directories(include/)

set(HEADER_FILES include/BaseSocket.h include/Error.h include/Handler.h include/Reactor.h include/SocketAddress.h)
set(SOURCE_FILES src/Handler.cpp src/Reactor.cpp src/BaseSocket.cpp src/SocketAddress.cpp)

add_library(sockets ${HEADER_FILES} ${SOURCE_FILES})
