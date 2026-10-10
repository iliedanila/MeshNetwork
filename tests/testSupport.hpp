#ifndef testSupport_hpp
#define testSupport_hpp

#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

namespace MeshTest {

using namespace std::chrono_literals;

class TestFailure : public std::runtime_error {
   public:
    using std::runtime_error::runtime_error;
};

#define MESH_CHECK(condition)                                             \
    do {                                                                  \
        if (!(condition)) {                                               \
            throw MeshTest::TestFailure(std::string(__FILE__) + ":" +     \
                                        std::to_string(__LINE__) + ": " + \
                                        #condition);                      \
        }                                                                 \
    } while (false)

// Runs the loop in short slices until `predicate` holds or `timeout` passes.
inline bool runUntil(boost::asio::io_context& io,
                     const std::function<bool()>& predicate,
                     std::chrono::milliseconds timeout = 5s) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!predicate()) {
        if (std::chrono::steady_clock::now() >= deadline) {
            return false;
        }
        io.restart();
        io.run_for(10ms);
    }
    return true;
}

// Runs the loop for `window` and returns how many handlers ran.
inline std::size_t handlersRunIn(boost::asio::io_context& io,
                                 std::chrono::milliseconds window) {
    io.restart();
    return io.run_for(window);
}

// Waits for a window in which the mesh does (almost) nothing. An idle mesh
// only has pending reads, which run no handlers.
inline bool runUntilQuiet(boost::asio::io_context& io,
                          std::chrono::milliseconds timeout = 5s,
                          std::chrono::milliseconds window = 300ms) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (handlersRunIn(io, window) <= 2) {
            return true;
        }
    }
    return false;
}

// Captures std::cout, where the logger node prints received logs.
class CoutCapture {
   public:
    CoutCapture() : previous(std::cout.rdbuf(captured.rdbuf())) {}

    ~CoutCapture() { std::cout.rdbuf(previous); }

    bool contains(const std::string& text) const {
        return captured.str().find(text) != std::string::npos;
    }

   private:
    std::stringstream captured;
    std::streambuf* previous;
};

using TestFunction = std::function<void()>;

inline int runTests(const std::map<std::string, TestFunction>& tests, int argc,
                    char** argv) {
    if (argc > 1 && tests.count(argv[1]) == 0) {
        std::cerr << "FAIL unknown test " << argv[1] << "\n";
        return 1;
    }
    int failures = 0;
    for (const auto& [name, test] : tests) {
        if (argc > 1 && name != argv[1]) {
            continue;
        }
        try {
            test();
            std::cerr << "PASS " << name << "\n";
        } catch (const std::exception& error) {
            std::cerr << "FAIL " << name << ": " << error.what() << "\n";
            ++failures;
        }
    }
    return failures == 0 ? 0 : 1;
}

}  // namespace MeshTest

#endif /* testSupport_hpp */
