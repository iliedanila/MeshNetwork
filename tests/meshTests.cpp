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

bool seesAll(Node& node, std::initializer_list<std::string> names) {
    for (const auto& name : names) {
        if (!node.isNodeAccessible(name)) {
            return false;
        }
    }
    return true;
}

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

struct SendResult {
    std::string nodeName;
    SendError error;
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

void nodeConnectsToSeveralPeers() {
    io_context io;
    Node a("A", io), b("B", io), c("C", io);
    a.accept(47030);
    b.accept(47031);

    c.connect("localhost", 47030);
    c.connect("localhost", 47031);
    MESH_CHECK(runUntil(io, [&] { return seesAll(c, {"A", "B"}); }, 3s));
}

void loggerPrintsItsOwnLogs() {
    CoutCapture out;
    io_context io;
    Node logger("L", io, true);

    logger.log("hello");
    MESH_CHECK(runUntil(io, [&] { return out.contains("-- L -- hello"); }, 1s));
}

void lateJoinerLearnsLogger() {
    CoutCapture out;
    io_context io;
    Node logger("L", io, true), a("A", io), b("B", io);
    logger.accept(47050);
    a.accept(47051);

    connectAndWait(io, a, 47050, logger);
    connectAndWait(io, b, 47051, a);
    MESH_CHECK(runUntil(io, [&] { return b.isNodeAccessible("L"); }));
    MESH_CHECK(runUntil(
        io,
        [&] {
            b.log("from B");
            return out.contains("-- B -- from B");
        },
        2s));
}

void loggerRouteRecoversAfterLoggerRestarts() {
    CoutCapture out;
    io_context io;
    auto logger = std::make_unique<Node>("L", io, true);
    logger->accept(47060);
    Node x("X", io), y("Y", io);
    x.accept(47061);

    connectAndWait(io, y, 47061, x);
    x.connect("localhost", 47060, true);
    MESH_CHECK(runUntil(io, [&] { return y.isNodeAccessible("L"); }));
    MESH_CHECK(runUntil(io, [&] {
        y.log("before");
        return out.contains("-- Y -- before");
    }));

    logger->close();
    MESH_CHECK(runUntil(io, [&] {
        return !x.isNodeAccessible("L") && !y.isNodeAccessible("L");
    }));
    handlersRunIn(io, 200ms);
    logger = std::make_unique<Node>("L", io, true);
    logger->accept(47060);

    MESH_CHECK(runUntil(io, [&] { return y.isNodeAccessible("L"); }, 4s));
    MESH_CHECK(runUntil(
        io,
        [&] {
            y.log("after");
            return out.contains("-- Y -- after");
        },
        2s));
}

void noSelfRoutesInTriangle() {
    io_context io;
    Node a("A", io), b("B", io), c("C", io);
    a.accept(47070);
    b.accept(47071);
    c.accept(47072);

    connectAndWait(io, a, 47071, b);
    connectAndWait(io, b, 47072, c);
    MESH_CHECK(runUntil(io, [&] { return a.isNodeAccessible("C"); }));
    connectAndWait(io, c, 47070, a);
    MESH_CHECK(runUntilQuiet(io));

    for (Node* node : {&a, &b, &c}) {
        MESH_CHECK(!node->isNodeAccessible(node->getName()));
        MESH_CHECK(node->getAccessibleNodes().size() == 2);
    }
}

void triangleWithLoggerGoesQuiet() {
    io_context io;
    Node logger("L", io, true), x("X", io), y("Y", io);
    logger.accept(47080);
    x.accept(47081);
    y.accept(47082);

    connectAndWait(io, x, 47080, logger);
    connectAndWait(io, y, 47081, x);
    connectAndWait(io, logger, 47082, y);
    MESH_CHECK(runUntilQuiet(io, 3s));
}

void departedNodeIsForgottenInRing() {
    io_context io;
    Node a("A", io), b("B", io), c("C", io), d("D", io), x("X", io);
    a.accept(47090);
    b.accept(47091);
    c.accept(47092);
    d.accept(47093);

    connectAndWait(io, a, 47091, b);
    connectAndWait(io, b, 47092, c);
    connectAndWait(io, c, 47093, d);
    connectAndWait(io, d, 47090, a);
    connectAndWait(io, x, 47090, a);
    MESH_CHECK(runUntil(io, [&] {
        return seesAll(a, {"X"}) && seesAll(b, {"X"}) && seesAll(c, {"X"}) &&
               seesAll(d, {"X"});
    }));
    MESH_CHECK(runUntilQuiet(io));

    x.close();
    MESH_CHECK(runUntilQuiet(io, 5s));
    for (Node* node : {&a, &b, &c, &d}) {
        MESH_CHECK(!node->isNodeAccessible("X"));
        MESH_CHECK(node->getAccessibleNodes().size() == 3);
    }
}

void notAcceptingDestinationReportsError() {
    io_context io;
    Node a("A", io), b("B", io), c("C", io);  // C never calls acceptMessages
    b.accept(47101);
    c.accept(47102);
    connectAndWait(io, a, 47101, b);
    connectAndWait(io, b, 47102, c);
    MESH_CHECK(runUntil(io, [&] { return a.isNodeAccessible("C"); }));

    std::optional<SendResult> result;
    a.sndMessage("C", "hi", [&](std::string nodeName, SendError error) {
        result = SendResult{nodeName, error};
    });
    MESH_CHECK(runUntil(io, [&] { return result.has_value(); }, 2s));
    MESH_CHECK(result->nodeName == "C");
    MESH_CHECK(result->error == NetworkLayer::eNodeNotAccepting);
}

void sendFailsWhenDestinationDisappears() {
    io_context io;
    Node a("A", io), b("B", io), c("C", io);
    c.acceptMessages([](DataMessage&) {});
    b.accept(47111);
    c.accept(47112);
    connectAndWait(io, a, 47111, b);
    connectAndWait(io, b, 47112, c);
    MESH_CHECK(runUntil(io, [&] { return a.isNodeAccessible("C"); }));

    std::optional<SendResult> result;
    c.close();
    a.sndMessage("C", "hi", [&](std::string nodeName, SendError error) {
        result = SendResult{nodeName, error};
    });
    MESH_CHECK(runUntil(io, [&] { return result.has_value(); }, 3s));
    MESH_CHECK(result->nodeName == "C");
    MESH_CHECK(result->error == NetworkLayer::eNoPath);
}

void identicalPayloadsAreEachAcknowledged() {
    io_context io;
    Node a("A", io), b("B", io);
    b.acceptMessages([](DataMessage&) {});
    b.accept(47121);
    connectAndWait(io, a, 47121, b);

    int acknowledged = 0;
    for (int i = 0; i < 2; ++i) {
        a.sndMessage("B", "same", [&](std::string, SendError error) {
            if (error == NetworkLayer::eSuccess) {
                ++acknowledged;
            }
        });
    }
    MESH_CHECK(runUntil(io, [&] { return acknowledged == 2; }, 2s));
}

void oversizedSendReportsError() {
    io_context io;
    Node a("A", io), b("B", io);
    b.acceptMessages([](DataMessage&) {});
    b.accept(47131);
    connectAndWait(io, a, 47131, b);

    std::optional<SendResult> result;
    a.sndMessage("B", std::string(17 * 1024 * 1024, 'x'),
                 [&](std::string nodeName, SendError error) {
                     result = SendResult{nodeName, error};
                 });
    MESH_CHECK(runUntil(io, [&] { return result.has_value(); }, 2s));
    MESH_CHECK(result->nodeName == "B");
    MESH_CHECK(result->error == NetworkLayer::eMessageTooLarge);
    MESH_CHECK(a.isNodeAccessible("B"));
}

// A neighbour's advert that was in flight while the neighbour rerouted must
// not survive the destination leaving.
void staleAdvertIsWithdrawnAfterReroute() {
    io_context io;
    Node a("A", io), b("B", io), c("C", io), d("D", io), e("E", io);
    a.accept(47140);
    b.accept(47141);
    c.accept(47142);
    d.accept(47143);
    TcpRelay eToA(io, 47144, 47140);
    e.connect("localhost", 47144);
    MESH_CHECK(runUntil(io, [&] {
        return a.isNodeAccessible("E") && e.isNodeAccessible("A");
    }));

    // E learns B the long way round; its advert to A waits in the relay.
    eToA.holdClientToTarget(true);
    connectAndWait(io, e, 47143, d);
    connectAndWait(io, d, 47142, c);
    connectAndWait(io, c, 47141, b);
    MESH_CHECK(runUntil(io, [&] { return e.isNodeAccessible("B"); }));
    MESH_CHECK(!a.isNodeAccessible("B"));

    // A links to B directly, so E reroutes to B through A.
    connectAndWait(io, a, 47141, b);
    MESH_CHECK(runUntilQuiet(io));

    b.close();
    MESH_CHECK(runUntilQuiet(io));
    eToA.holdClientToTarget(false);
    MESH_CHECK(runUntilQuiet(io));
    for (Node* node : {&a, &c, &d, &e}) {
        MESH_CHECK(!node->isNodeAccessible("B"));
    }
}

void duplicateLinkSurvivesLosingTheOther() {
    io_context io;
    Node a("A", io), b("B", io);
    a.accept(47150);
    TcpRelay relay(io, 47151, 47150);
    connectAndWait(io, b, 47151, a);
    b.connect("localhost", 47150);
    MESH_CHECK(runUntilQuiet(io));

    relay.cut();
    MESH_CHECK(runUntilQuiet(io));
    MESH_CHECK(a.isNodeAccessible("B"));
    MESH_CHECK(b.isNodeAccessible("A"));
}

}  // namespace

int main(int argc, char** argv) {
    return runTests(
        {
            {"malformedFrameClosesOnlyThatConnection",
             malformedFrameClosesOnlyThatConnection},
            {"oversizedFrameIsRejected", oversizedFrameIsRejected},
            {"nodeConnectsToSeveralPeers", nodeConnectsToSeveralPeers},
            {"loggerPrintsItsOwnLogs", loggerPrintsItsOwnLogs},
            {"lateJoinerLearnsLogger", lateJoinerLearnsLogger},
            {"loggerRouteRecoversAfterLoggerRestarts",
             loggerRouteRecoversAfterLoggerRestarts},
            {"noSelfRoutesInTriangle", noSelfRoutesInTriangle},
            {"triangleWithLoggerGoesQuiet", triangleWithLoggerGoesQuiet},
            {"departedNodeIsForgottenInRing", departedNodeIsForgottenInRing},
            {"notAcceptingDestinationReportsError",
             notAcceptingDestinationReportsError},
            {"sendFailsWhenDestinationDisappears",
             sendFailsWhenDestinationDisappears},
            {"identicalPayloadsAreEachAcknowledged",
             identicalPayloadsAreEachAcknowledged},
            {"oversizedSendReportsError", oversizedSendReportsError},
            {"staleAdvertIsWithdrawnAfterReroute",
             staleAdvertIsWithdrawnAfterReroute},
            {"duplicateLinkSurvivesLosingTheOther",
             duplicateLinkSurvivesLosingTheOther},
        },
        argc, argv);
}
