#include <array>
#include <boost/asio.hpp>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "node.hpp"
#include "testSupport.hpp"

using namespace MeshTest;
using boost::asio::io_context;
using boost::asio::ip::tcp;
using NetworkLayer::DataMessage;
using NetworkLayer::Node;
using NetworkLayer::SendError;

namespace {

void connectAndWait(io_context& io, Node& from, unsigned short port, Node& to) {
    from.connect("localhost", port);
    MESH_CHECK(runUntil(io, [&] {
        return from.isNodeAccessible(to.getName()) &&
               to.isNodeAccessible(from.getName());
    }));
}

// A TCP client that speaks no protocol, for feeding a node raw bytes.
class RawClient {
   public:
    RawClient(io_context& io, unsigned short port) : socket(io) {
        socket.connect(
            tcp::endpoint(boost::asio::ip::address_v4::loopback(), port));
        waitForClose();
    }

    void send(const std::string& bytes) {
        boost::asio::write(socket, boost::asio::buffer(bytes));
    }

    bool closedByPeer() const { return closed; }

   private:
    void waitForClose() {
        socket.async_read_some(
            boost::asio::buffer(readBuffer),
            [this](boost::system::error_code error, std::size_t /*length*/) {
                if (error) {
                    closed = true;
                } else {
                    waitForClose();
                }
            });
    }

    tcp::socket socket;
    std::array<char, 256> readBuffer{};
    bool closed = false;
};

void malformedFrameClosesOnlyThatConnection() {
    io_context io;
    Node a("A", io);
    a.accept(47010);
    Node b("B", io);

    RawClient raw(io, 47010);
    raw.send(
        std::string("\x00\x00\x00\x03"
                    "abc",
                    7));
    MESH_CHECK(runUntil(io, [&] { return raw.closedByPeer(); }));

    connectAndWait(io, b, 47010, a);
}

void oversizedFrameIsRejected() {
    io_context io;
    Node a("A", io);
    a.accept(47020);

    RawClient raw(io, 47020);
    raw.send(std::string("\x04\x00\x00\x00", 4));  // announces 64 MiB
    MESH_CHECK(runUntil(io, [&] { return raw.closedByPeer(); }, 2s));
}

}  // namespace

int main(int argc, char** argv) {
    return runTests(
        {
            {"malformedFrameClosesOnlyThatConnection",
             malformedFrameClosesOnlyThatConnection},
            {"oversizedFrameIsRejected", oversizedFrameIsRejected},
        },
        argc, argv);
}
