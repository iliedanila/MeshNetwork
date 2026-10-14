#include "node.hpp"
#include "allMessages.hpp"
#include "connection.hpp"

#include <algorithm>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <variant>

using namespace boost::asio::ip;

namespace NetworkLayer {

namespace {

bool pathContains(const std::vector<std::string>& path,
                  const std::string& nodeName) {
    return std::find(path.begin(), path.end(), nodeName) != path.end();
}

void printLog(int64_t millisecondsSinceEpoch, const std::string& source,
              const std::string& text) {
    std::chrono::time_point<std::chrono::system_clock> epoch;
    auto logTime = std::chrono::system_clock::to_time_t(
        epoch + std::chrono::milliseconds{millisecondsSinceEpoch});

    std::cout << "LOG -- " << std::put_time(std::localtime(&logTime), "%F %T")
              << " -- " << source << " -- " << text << "\n";
}

}  // namespace

Node::Node(std::string _name, boost::asio::io_context& _ioservice,
           bool _isLogger)
    : name(std::move(_name)),
      closing(false),
      accept_socket(_ioservice),
      ioservice(_ioservice),
      isLogger(_isLogger) {}

Node::~Node() {}

void Node::accept(unsigned short _port) {
    tcp::endpoint endpoint(tcp::v4(), _port);

    if (!acceptor) {
        acceptor = std::make_unique<tcp::acceptor>(ioservice, endpoint);
    }

    acceptor->async_accept(
        accept_socket, [this, _port](const boost::system::error_code& error) {
            if (!error) {
                auto connection =
                    addConnection(std::move(accept_socket), std::nullopt);

                startConnection(connection, Connection::eAccepted);
            } else {
                std::cout << name << " error accepting: " << error.message()
                          << "\n";
            }
            if (!closing) {
                accept(_port);
            }
        });
}

// Each attempt owns its resolver and socket, so several connects can be in
// flight at once, and name resolution never blocks the loop.
void Node::connect(std::string host, unsigned short port, bool reconnect) {
    auto resolver = std::make_shared<tcp::resolver>(ioservice);
    resolver->async_resolve(
        host, std::to_string(port),
        [this, resolver, host, port, reconnect](
            const boost::system::error_code& error,
            tcp::resolver::results_type endpoints) {
            if (error) {
                onConnectFailed(host, port, reconnect, error);
                return;
            }
            auto socket = std::make_shared<tcp::socket>(ioservice);
            boost::asio::async_connect(
                *socket, endpoints,
                [this, socket, host, port, reconnect](
                    const boost::system::error_code& error,
                    const tcp::endpoint& /*endpoint*/) {
                    if (error) {
                        onConnectFailed(host, port, reconnect, error);
                        return;
                    }
                    if (closing) {
                        return;
                    }
                    std::optional<ReconnectTarget> reconnectTarget;
                    if (reconnect) {
                        reconnectTarget = ReconnectTarget{host, port};
                    }
                    auto connection =
                        addConnection(std::move(*socket), reconnectTarget);
                    startConnection(connection, Connection::eConnected);
                });
        });
}

void Node::onConnectFailed(const std::string& host, unsigned short port,
                           bool reconnect,
                           const boost::system::error_code& error) {
    std::cout << name << " error connecting: " << error.message() << "\n";
    if (!reconnect || closing) {
        return;
    }
    std::cout << "Reconnecting...\n";
    auto reconnectTimer =
        std::make_shared<boost::asio::steady_timer>(ioservice);
    reconnectTimer->expires_after(std::chrono::seconds(1));
    reconnectTimer->async_wait(std::bind(&Node::onReconnectTimer, this,
                                         reconnectTimer, host, port,
                                         std::placeholders::_1));
}

void Node::close() {
    closing = true;
    for (auto connection : connections) {
        connection->close();
    }
    if (acceptor) {
        acceptor->cancel();
    }
}

std::vector<std::string> Node::getAccessibleNodes() {
    std::vector<std::string> nodeList;
    for (const auto& [destination, route] : routes) {
        nodeList.push_back(destination);
    }
    return nodeList;
}

bool Node::isNodeAccessible(const std::string& nodeName) {
    return routes.find(nodeName) != routes.end();
}

void Node::notifyNewNodeStatus(
    std::function<void(std::string, bool)> callback) {
    notifyNewNodeStatusCallback = callback;
}

void Node::sndMessage(std::string destination, std::string data,
                      std::function<void(std::string, SendError)> callback) {
    auto connection = getConnectionToNode(destination);
    if (!connection) {
        callback(destination, eNoPath);
        return;
    }

    auto messageID = ++lastMessageID;
    pendingSends[messageID] = PendingSend{destination, callback};
    DataMessage message(name, destination, std::move(data), messageID);
    connection->send(message,
                     [this, messageID](boost::system::error_code error) {
                         if (error == boost::asio::error::message_size) {
                             completePendingSend(messageID, eMessageTooLarge);
                         }
                     });
}

void Node::acceptMessages(std::function<void(DataMessage&)> callback) {
    messageAcceptor = callback;
}

void Node::closeConnection(SharedConnection connectionDown) {
    RoutingMessage nodesDown;
    std::vector<std::string> unreachable;
    for (const auto& [destination, route] : routes) {
        if (route.via == connectionDown) {
            unreachable.push_back(destination);
        }
    }
    for (const auto& destination : unreachable) {
        withdrawRoute(destination, nodesDown);
    }

    connectionDown->close();
    connections.erase(connectionDown);

    if (closing) {
        return;
    }

    for (auto connection : connections) {
        connection->send(nodesDown, std::bind(&Node::onWrite, this, nodesDown,
                                              std::placeholders::_1));
    }

    if (const auto& target = connectionDown->getReconnectTarget()) {
        connect(target->host, target->port, true);
    }
}

void Node::sendRoutingToNewConnection(SharedConnection connection) {
    RoutingMessage message;
    message.addAdvert(RouteAdvert{{name}, isLogger});
    for (const auto& [destination, route] : routes) {
        message.addAdvert(advertise(route));
    }

    connection->send(message, std::bind(&Node::onWrite, this, message,
                                        std::placeholders::_1));
}

SharedConnection Node::addConnection(
    tcp::socket&& socket, std::optional<ReconnectTarget> reconnectTarget) {
    auto connection = std::make_shared<Connection>(
        *this, ioservice, std::move(socket), std::move(reconnectTarget),
        std::bind(&Node::closeConnection, this, std::placeholders::_1));

    connections.insert(connection);

    return connection;
}

void Node::onRead(MessageVariant _message, SharedConnection _connection) {
    // A frame read just before its connection closed must not install routes
    // through a connection that is gone.
    if (connections.count(_connection) == 0) {
        return;
    }
    std::visit([&](auto& message) { handleMessage(message, _connection); },
               _message);
}

void Node::onWrite(MessageVariant message,
                   boost::system::error_code error) const {}

void Node::onReconnectTimer(std::shared_ptr<boost::asio::steady_timer> timer,
                            std::string host, unsigned short port,
                            const boost::system::error_code& error) {
    if (!error && !closing) {
        connect(host, port, true);
    }
}

void Node::handleMessage(RoutingMessage& _message,
                         SharedConnection _connection) {
    RoutingMessage reply;
    RoutingMessage forward;

    processAdverts(_message, reply, forward, _connection);
    processFailedNodes(_message, reply, forward, _connection);

    if (!reply.empty()) {
        _connection->send(reply, std::bind(&Node::onWrite, this, reply,
                                           std::placeholders::_1));
    }

    // The sender gets the changes too: it may still hold an advert of ours
    // that this change makes stale.
    if (!forward.empty()) {
        for (auto conn : connections) {
            conn->send(forward, std::bind(&Node::onWrite, this, forward,
                                          std::placeholders::_1));
        }
    }
}

void Node::handleMessage(DataMessage& _message, SharedConnection _connection) {
    _message.distance++;

    if (_message.destinationNodeName == name) {
        SendError result = eNodeNotAccepting;
        if (messageAcceptor) {
            messageAcceptor(_message);
            result = eSuccess;
        }
        DataMessageAck message(name, _message.sourceNodeName, result,
                               _message.getMessageID());
        _connection->send(message, std::bind(&Node::onWrite, this, message,
                                             std::placeholders::_1));
    } else {
        if (auto connection =
                getConnectionToNode(_message.destinationNodeName)) {
            connection->send(_message, std::bind(&Node::onWrite, this, _message,
                                                 std::placeholders::_1));
        } else {
            DataMessageAck message(name, _message.sourceNodeName, eNoPath,
                                   _message.getMessageID());
            _connection->send(message, std::bind(&Node::onWrite, this, message,
                                                 std::placeholders::_1));
        }
    }
}

void Node::handleMessage(DataMessageAck& _message,
                         SharedConnection _connection) {
    if (_message.destinationNodeName == name) {
        completePendingSend(_message.messageID, _message.error);
    } else {
        if (auto connection =
                getConnectionToNode(_message.destinationNodeName)) {
            connection->send(_message, std::bind(&Node::onWrite, this, _message,
                                                 std::placeholders::_1));
        }
    }
}

void Node::handleMessage(LogMessage& _message, SharedConnection _connection) {
    if (_message.getDestinationNodeName() == name) {
        printLog(_message.getMillisecondsSinceEpoch(),
                 _message.getSourceNodeName(), _message.getLogMessage());
    } else {
        auto forwardConnection =
            getConnectionToNode(_message.getDestinationNodeName());
        if (forwardConnection) {
            boost::asio::post(ioservice, [this, forwardConnection, _message] {
                forwardConnection->send(
                    _message, std::bind(&Node::onWrite, this, _message,
                                        std::placeholders::_1));
            });
        }
    }
}

void Node::handleMessage(Handshake& _message, SharedConnection _connection) {
    // this is the accepted connection.
    std::cout << name << ": connection with node " << _message.getNodeName()
              << " established.\n";
    HandshakeReply message(name);
    _connection->send(message,
                      [this, _connection](boost::system::error_code ec) {
                          if (!ec) {
                              sendRoutingToNewConnection(_connection);
                          }
                      });
}

void Node::handleMessage(HandshakeReply& _message,
                         SharedConnection _connection) {
    // this is the connected connection.
    std::cout << name << ": connection with node " << _message.getNodeName()
              << " established.\n";
    sendRoutingToNewConnection(_connection);
}

void Node::processAdverts(const RoutingMessage& message, RoutingMessage& reply,
                          RoutingMessage& forward,
                          SharedConnection connection) {
    for (const auto& advert : message.getAdverts()) {
        if (advert.path.empty()) {
            continue;
        }
        const auto& destination = advert.destination();
        auto it = routes.find(destination);

        if (pathContains(advert.path, name)) {
            // The neighbour now routes through us, so the route it gave us
            // earlier is gone. Adverts for ourselves end up here too.
            if (it != routes.end() && it->second.via == connection) {
                withdrawRoute(destination, forward);
            }
            continue;
        }

        if (it == routes.end()) {
            auto& route = routes[destination];
            route = Route{advert.path, connection, advert.isLogger};
            forward.addAdvert(advertise(route));
            if (notifyNewNodeStatusCallback) {
                boost::asio::post(
                    ioservice,
                    std::bind(notifyNewNodeStatusCallback, destination, true));
            }
        } else if (it->second.via == connection ||
                   advert.path.size() < it->second.path.size()) {
            // Our next hop's current route replaces what it told us before;
            // anyone else's route only wins by being shorter.
            Route updated{advert.path, connection, advert.isLogger};
            if (updated.path != it->second.path ||
                updated.via != it->second.via ||
                updated.isLogger != it->second.isLogger) {
                it->second = updated;
                forward.addAdvert(advertise(it->second));
            }
        } else if (it->second.path.size() + 2 < advert.path.size()) {
            // Going through us would be shorter for the neighbour (its advert
            // counts itself, ours would count us).
            reply.addAdvert(advertise(it->second));
        }
    }
}

void Node::processFailedNodes(const RoutingMessage& message,
                              RoutingMessage& reply, RoutingMessage& forward,
                              SharedConnection connection) {
    for (const auto& node : message.getFailedNodes()) {
        if (node == name) {
            // The neighbour lost its route to us but still has this link.
            reply.addAdvert(RouteAdvert{{name}, isLogger});
            continue;
        }
        auto it = routes.find(node);
        if (it == routes.end()) {
            continue;
        }
        if (it->second.via == connection) {
            withdrawRoute(node, forward);
        } else {
            reply.addAdvert(advertise(it->second));
        }
    }
}

void Node::withdrawRoute(const std::string& destination,
                         RoutingMessage& forward) {
    routes.erase(destination);
    forward.addFailedNode(destination);
    failPendingSends(destination);
    // Posted like the "reachable" notice, so the two arrive in order.
    if (notifyNewNodeStatusCallback) {
        boost::asio::post(ioservice, std::bind(notifyNewNodeStatusCallback,
                                               destination, false));
    }
}

RouteAdvert Node::advertise(const Route& route) const {
    RouteAdvert advert{{name}, route.isLogger};
    advert.path.insert(advert.path.end(), route.path.begin(), route.path.end());
    return advert;
}

// Reports a send's outcome once; whichever of the ack or a route failure
// arrives second finds nothing to report.
void Node::completePendingSend(uint64_t messageID, SendError error) {
    auto it = pendingSends.find(messageID);
    if (it == pendingSends.end()) {
        return;
    }
    auto pending = std::move(it->second);
    pendingSends.erase(it);
    pending.callback(pending.destination, error);
}

// The message or its ack may have been on the lost route; without a route
// the sender would otherwise never hear back.
void Node::failPendingSends(const std::string& destination) {
    for (const auto& [messageID, pending] : pendingSends) {
        if (pending.destination == destination) {
            boost::asio::post(ioservice, [this, messageID = messageID] {
                completePendingSend(messageID, eNoPath);
            });
        }
    }
}

const std::string* Node::nearestLogger() const {
    const std::string* nearest = nullptr;
    std::size_t nearestHops = 0;
    for (const auto& [destination, route] : routes) {
        if (route.isLogger &&
            (nearest == nullptr || route.path.size() < nearestHops)) {
            nearest = &destination;
            nearestHops = route.path.size();
        }
    }
    return nearest;
}

void Node::startConnection(SharedConnection connection, Connection::Type type) {
    if (type == Connection::eConnected) {
        Handshake message(name);
        connection->send(
            message, [this, connection](boost::system::error_code ec) {
                if (!ec) {
                    // Wait for reply.
                    connection->read(std::bind(&Node::onRead, this,
                                               std::placeholders::_1,
                                               std::placeholders::_2));
                }
            });
    } else if (type == Connection::eAccepted) {
        // waiting for handshake message.
        connection->read(std::bind(&Node::onRead, this, std::placeholders::_1,
                                   std::placeholders::_2));
    }
}

void Node::log(const std::string& logMessage) {
    auto millisecondsSinceEpoch =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    if (isLogger) {
        printLog(millisecondsSinceEpoch, name, logMessage);
        return;
    }

    const std::string* logger = nearestLogger();
    if (logger == nullptr) {
        return;
    }
    LogMessage message(millisecondsSinceEpoch, name, *logger, logMessage);
    auto connection = getConnectionToNode(*logger);
    boost::asio::post(ioservice, [this, connection, message] {
        connection->send(message, std::bind(&Node::onWrite, this, message,
                                            std::placeholders::_1));
    });
}

SharedConnection Node::getConnectionToNode(const std::string& nodeName) {
    auto it = routes.find(nodeName);
    return it == routes.end() ? nullptr : it->second.via;
}
}  // namespace NetworkLayer
