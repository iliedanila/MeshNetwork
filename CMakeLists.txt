cmake_minimum_required(VERSION 3.25...4.4)

project(MeshNetwork LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# No C++20 modules here, so skip the dependency scan CMake 3.28+ runs on
# every C++20 source.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

find_package(Boost CONFIG REQUIRED COMPONENTS serialization)
find_package(Threads REQUIRED)

add_library(MeshNetwork STATIC
    src/connection.cpp
    src/node.cpp
    inc/allMessages.hpp
    inc/connection.hpp
    inc/dataMessage.hpp
    inc/dataMessageAck.hpp
    inc/handshake.hpp
    inc/handshakeReply.hpp
    inc/logMessage.hpp
    inc/message.hpp
    inc/messageVisitor.hpp
    inc/node.hpp
    inc/routingMessage.hpp
    inc/sendError.hpp
)
# The headers include Boost.Asio and Boost.Serialization, so anything that
# links MeshNetwork needs them too, and C++20 with them.
target_include_directories(MeshNetwork PUBLIC inc)
target_compile_features(MeshNetwork PUBLIC cxx_std_20)
target_link_libraries(MeshNetwork
    PUBLIC Boost::headers Boost::serialization Threads::Threads)

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
endif()
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
