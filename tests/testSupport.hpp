#ifndef testSupport_hpp
#define testSupport_hpp

#include <array>
#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
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

// Relays one TCP connection from `listenPort` to `targetPort`. Holding a
// direction queues its bytes until released, which stages message orders;
// cutting closes both ends, like a dropped link.
class TcpRelay {
   public:
    TcpRelay(boost::asio::io_context& io, unsigned short listenPort,
             unsigned short targetPort)
        : acceptor(io, {boost::asio::ip::address_v4::loopback(), listenPort}),
          client(io),
          target(io) {
        acceptor.async_accept(
            client, [this, targetPort](boost::system::error_code error) {
                if (error) {
                    return;
                }
                target.async_connect(
                    {boost::asio::ip::address_v4::loopback(), targetPort},
                    [this](boost::system::error_code error) {
                        if (error) {
                            cut();
                            return;
                        }
                        pump(clientToTarget);
                        pump(targetToClient);
                    });
            });
    }

    void holdClientToTarget(bool hold) {
        clientToTarget.holding = hold;
        flush(clientToTarget);
    }

    void cut() {
        boost::system::error_code ignored;
        client.close(ignored);
        target.close(ignored);
    }

   private:
    struct Direction {
        boost::asio::ip::tcp::socket& from;
        boost::asio::ip::tcp::socket& to;
        std::array<char, 4096> readBuffer{};
        std::string pending;
        bool holding = false;
        bool writing = false;
    };

    void pump(Direction& direction) {
        direction.from.async_read_some(
            boost::asio::buffer(direction.readBuffer),
            [this, &direction](boost::system::error_code error,
                               std::size_t length) {
                if (error) {
                    cut();
                    return;
                }
                direction.pending.append(direction.readBuffer.data(), length);
                flush(direction);
                pump(direction);
            });
    }

    void flush(Direction& direction) {
        if (direction.holding || direction.writing ||
            direction.pending.empty()) {
            return;
        }
        direction.writing = true;
        auto chunk =
            std::make_shared<std::string>(std::move(direction.pending));
        direction.pending.clear();
        boost::asio::async_write(
            direction.to, boost::asio::buffer(*chunk),
            [this, &direction, chunk](boost::system::error_code error,
                                      std::size_t /*length*/) {
                direction.writing = false;
                if (error) {
                    cut();
                    return;
                }
                flush(direction);
            });
    }

    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::ip::tcp::socket client;
    boost::asio::ip::tcp::socket target;
    Direction clientToTarget{client, target};
    Direction targetToClient{target, client};
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
