# MeshNetwork
A network of self-routing nodes. Used to transfer data asynchronously over TCP.

In order to run this, you'll need CMake 3.25 or newer, a C++20 compiler and `boost` with its compiled
`serialization` library and the CMake package files Boost installs, from a package manager or from
http://www.boost.org/. If CMake doesn't find Boost, pass `-DCMAKE_PREFIX_PATH=<where Boost is installed>`.

Build and test on its own with
`cmake -S . -B build && cmake --build build && (cd build && ctest)`.

Example of usages can be found in https://github.com/iliedanila/PubSub
