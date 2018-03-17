cmake_minimum_required(VERSION 2.8.9)

project(MeshNetwork)

set (CMAKE_CXX_STANDARD 14)

include_directories(inc)
include_directories(boost/inc)

file(GLOB SOURCES "src/*.cpp")

add_library(MeshNetwork STATIC ${SOURCES})
