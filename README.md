# MeshNetwork
A network of self-routing nodes. Used to transfer data asynchronously over TCP.

In order to run this, you'll need a C++20 compiler and `boost` (latest version is recommended), download from http://www.boost.org/
and build it using the default output path: on windows it should be `C:\Boost` and on Unix `/usr/local`

Build and test on its own with
`cmake -S . -B build && cmake --build build && (cd build && ctest)`.

Example of usages can be found in https://github.com/iliedanila/PubSub
