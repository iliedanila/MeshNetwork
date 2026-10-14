cmake_minimum_required(VERSION 3.5)

project(MeshNetwork)

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(inc)
# include_directories(boost/inc)

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "inc/*.hpp")

add_library(MeshNetwork STATIC ${HEADERS} ${SOURCES})

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(CTest)
endif()
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
