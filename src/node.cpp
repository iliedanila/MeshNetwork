#include "node.hpp"
#include "connection.hpp"
#include "allMessages.hpp"
#include "messageVisitor.hpp"

#include <string>
#include <functional>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <boost/variant/apply_visitor.hpp>

using namespace boost::asio::ip;

namespace NetworkLayer
{

Node::Node(
    std::string _name,
    boost::asio::io_service& _ioservice,
    bool _isLogger)
:
    name(std::move(_name)),
    closing(false),
    connect_socket(_ioservice),
    accept_socket(_ioservice),
    ioservice(_ioservice),
    isLogger(_isLogger)
{}

Node::~Node()
{}

void Node::accept(unsigned short _port)
{
    tcp::endpoint endpoint(tcp::v4(), _port);
    
    if (!acceptor)
    {
        acceptor = std::make_unique<tcp::acceptor>(ioservice, endpoint);
    }
    
    acceptor->async_accept(
        accept_socket,
        [this, _port]
        (const boost::system::error_code& error)
        {
            if (!error)
            {
                auto connection = addConnection(std::move(accept_socket), false);

                startConnection(connection, Connection::eAccepted);
            }
            else
            {
                std::cout << name << " error accepting: "
                << error.message() << "\n";
            }
            if(!closing)
            {
                accept(_port);
            }
        });
}

void Node::connect(
        std::string host,
        unsigned short port,
        bool reconnect)
{
    tcp::resolver resolver(ioservice);
    auto endpoint = resolver.resolve(host, std::to_string(port));
    
    boost::asio::async_connect(
        connect_socket,
        endpoint,
        [this, reconnect, host, port]
        (const boost::system::error_code& error, tcp::endpoint)
        {
            if (!error)
            {
                auto connection = addConnection(std::move(connect_socket), reconnect);

                startConnection(connection, Connection::eConnected);
            }
            else
            {
                std::cout 
                    << name 
                    << " error connecting: "
                    << error.message() 
                    << "\n";
                if (reconnect && !closing)
                {
                    std::cout << "Reconnecting...\n";

                    auto reconnectTimer = std::make_shared<boost::asio::deadline_timer>(
                        ioservice);
                    reconnectTimer->expires_from_now(boost::posix_time::seconds(1));
                    reconnectTimer->async_wait(std::bind(
                            &Node::onReconnectTimer,
                        this,
                        reconnectTimer,
                        host,
                        port,
                        std::placeholders::_1
                    ));
                }
            }
        });
}

void Node::close()
{
    closing = true;
    for (auto connection : connections)
    {
        connection->close();
    }
    if (acceptor)
    {
        acceptor->cancel();
    }
}

std::vector<std::string> Node::getAccessibleNodes()
{
    std::vector<std::string> nodeList;
    for ( auto keyValue : nodeDistances )
    {
        nodeList.push_back(keyValue.first);
    }
    return nodeList;
}

bool Node::isNodeAccessible(const std::string &nodeName)
{
    return nodeDistances.find(nodeName) != nodeDistances.end();
}
    
void Node::notifyNewNodeStatus(std::function<void(std::string, bool)> callback)
{
    notifyNewNodeStatusCallback = callback;
}

void Node::sndMessage(
        std::string destination,
        std::string data,
        std::function<void(std::string, SendError)> callback)
{
    if (isNodeAccessible(destination))
    {
        auto namePath = nodePaths.find(destination);
        auto connection = namePath->second;
        
        std::size_t messageID = std::hash<std::string>{}(data);
        DataMessage message(name, destination, data, messageID);

        connection->send(
                message,
                std::bind(
                        &Node::onWrite,
                        this,
                        message,
                        std::placeholders::_1
                )
        );

        ackCallbacks.insert({ messageID, callback });
    }
    else
    {
        callback(destination, eNoPath);
    }
}
    
void Node::acceptMessages(std::function<void(DataMessage &)> callback)
{
    messageAcceptor = callback;
}

void Node::closeConnection(SharedConnection connectionDown)
{
    RoutingMessage nodesDown;
    std::vector<std::string> toDelete;
    for (auto nodePath : nodePaths)
    {
        if (nodePath.second == connectionDown)
        {
            nodesDown.addFailedNode(nodePath.first);
            toDelete.push_back(nodePath.first);
        }
    }
    
    for (auto nodeName : toDelete)
    {
        if (notifyNewNodeStatusCallback)
        {
            notifyNewNodeStatusCallback(nodeName, false);
        }
        nodePaths.erase(nodeName);
        nodeDistances.erase(nodeName);
    }

    connectionDown->close();
    connections.erase(connectionDown);
    

    if(closing)
    {
        return;
    }
    
    for (auto connection : connections)
    {
        connection->send(
                nodesDown,
                std::bind(
                        &Node::onWrite,
                        this,
                        nodesDown,
                        std::placeholders::_1
                )
        );
    }

    if (connectionDown->getReconnect())
    {
        connect(connectionDown->getRemoteIP(), connectionDown->getRemotePort(), true);
    }
}

void Node::sendRoutingToNewConnection(SharedConnection connection)
{
    RoutingMessage message;
    message.addNodeDistance(std::make_pair(name, 0));
    
    for (auto nodeDistance : nodeDistances)
    {
        message.addNodeDistance(nodeDistance);
    }

    if(isLogger)
    {
        message.addLogger(std::make_pair(name, 0));
    }

    connection->send(
            message,
            std::bind(
                    &Node::onWrite,
                    this,
                    message,
                    std::placeholders::_1
            )
    );
}

SharedConnection Node::addConnection(tcp::socket &&socket, bool reconnect)
{
    auto connection = std::make_shared<Connection>(
        *this,
        ioservice,
        std::move(socket),
        reconnect,
        std::bind(
                &Node::closeConnection,
            this,
            std::placeholders::_1));
    
    connections.insert(connection);

    return connection;
}

void Node::onRead(MessageVariant _message, SharedConnection _connection)
{
    boost::apply_visitor(MessageVisitor(*this, _connection), _message);
}

void Node::onWrite(MessageVariant message, boost::system::error_code error) const
{
}

void Node::onReconnectTimer(
        std::shared_ptr<boost::asio::deadline_timer> timer,
        std::string host,
        unsigned short port,
        const boost::system::error_code &error)
{
    if (!error)
    {
        connect(host, port, true);
    }
}

    template <>
void Node::handleMessage(RoutingMessage& _message, SharedConnection _connection)
{
    RoutingMessage reply;
    RoutingMessage forward;

    processAddNodePaths(_message, reply, forward, _connection);
    processFailedNodes(_message, reply, forward, _connection);
    processLogger(_message, reply, forward);

    if (reply.getNodeDistances().size() > 0 ||
            reply.getFailedNodes().size() > 0 ||
            reply.getLoggerDistance().first != "")
    {
        _connection->send(
                reply,
                std::bind(
                        &Node::onWrite,
                        this,
                        reply,
                        std::placeholders::_1
                )
        );
    }
    
    if (forward.getNodeDistances().size() > 0 ||
            forward.getFailedNodes().size() > 0 ||
            forward.getLoggerDistance().first != "")
    {
        for (auto conn : connections)
        {
            if (conn != _connection)
            {
                conn->send(
                        forward,
                        std::bind(
                                &Node::onWrite,
                                this,
                                forward,
                                std::placeholders::_1
                        )
                );
            }
        }
    }
}

template<>
void Node::handleMessage(DataMessage& _message, SharedConnection _connection)
{
    _message.distance++;

    if (_message.destinationNodeName == name)
    {
        if (messageAcceptor)
        {
            messageAcceptor(_message);
            DataMessageAck message(name, _message.sourceNodeName, eSuccess, _message.getMessageID());
            _connection->send(
                    message,
                    std::bind(
                            &Node::onWrite,
                            this,
                            message,
                            std::placeholders::_1
                    )
            );
        }
        else
        {
            DataMessageAck message(
                _message.sourceNodeName,
                name,
                eNodeNotAccepting,
                _message.getMessageID()
            );
            _connection->send(
                    message,
                    std::bind(
                            &Node::onWrite,
                            this,
                            message,
                            std::placeholders::_1
                    )
            );
        }
    }
    else
    {
        if (isNodeAccessible(_message.destinationNodeName))
        {
            auto namePath = nodePaths.find(_message.destinationNodeName);
            auto connection = namePath->second;
            connection->send(
                    _message,
                    std::bind(
                            &Node::onWrite,
                            this,
                            _message,
                            std::placeholders::_1
                    )
            );
        }
        else
        {
            DataMessageAck message(_message.sourceNodeName, name, eNoPath, _message.getMessageID());
            _connection->send(
                    message,
                    std::bind(
                            &Node::onWrite,
                            this,
                            message,
                            std::placeholders::_1
                    )
            );
        }
    }
}
    
template<>
void Node::handleMessage(DataMessageAck& _message, SharedConnection _connection)
{
    if (_message.destinationNodeName == name)
    {
        auto it = ackCallbacks.find(_message.messageID);
        if(it != ackCallbacks.end())
        {
            it->second(_message.sourceNodeName, _message.error);
            ackCallbacks.erase(it);
        }
    }
    else
    {
        if (isNodeAccessible(_message.destinationNodeName))
        {
            auto namePath = nodePaths.find(_message.destinationNodeName);
            auto connection = namePath->second;
            connection->send(
                    _message,
                    std::bind(
                            &Node::onWrite,
                            this,
                            _message,
                            std::placeholders::_1
                    )
            );
        }
    }
}

template<>
void Node::handleMessage(LogMessage& _message, SharedConnection _connection)
{
    if (_message.getDestinationNodeName() == name)
    {
        std::chrono::time_point<std::chrono::system_clock> epoch;
        std::chrono::milliseconds millisecondsSinceEpoch{_message.getMillisecondsSinceEpoch() };
        auto logTime = std::chrono::system_clock::to_time_t(epoch + millisecondsSinceEpoch);

        std::cout
            << "LOG -- "
            << std::put_time(std::localtime(&logTime), "%F %T")
            << " -- "
            << _message.getSourceNodeName()
            << " -- "
            << _message.getLogMessage()
            << "\n";
    }
    else
    {
        auto forwardConnection = getConnectionToNode(_message.getDestinationNodeName());
        if (forwardConnection)
        {
            ioservice.post(
                [this, forwardConnection, _message]
                {
                    forwardConnection->send(
                            _message,
                            std::bind(
                                    &Node::onWrite,
                                    this,
                                    _message,
                                    std::placeholders::_1
                            )
                    );
                }
            );
        }
    }
}

template<>
void Node::handleMessage(Handshake& _message, SharedConnection _connection)
{
    // this is the accepted connection.
    std::cout << name << ": connection with node " << _message.getNodeName() << " established.\n";
    HandshakeReply message(name);
    _connection->send(message, [this, _connection](boost::system::error_code ec) {
        if (!ec) {
            sendRoutingToNewConnection(_connection);
        }
    });
}

template<>
void Node::handleMessage(HandshakeReply& _message, SharedConnection _connection)
{
    // this is the connected connection.
    std::cout << name << ": connection with node " << _message.getNodeName() << " established.\n";
    sendRoutingToNewConnection(_connection);
}

void Node::processAddNodePaths(
        RoutingMessage &message,
        RoutingMessage &reply,
        RoutingMessage &forward,
        SharedConnection connection)
{
    for (auto nodeDistance : message.getNodeDistances())
    {
        auto it = nodeDistances.find(nodeDistance.first);
        if (it == nodeDistances.end())
        {
            auto nodeDist = std::make_pair(nodeDistance.first, nodeDistance.second + 1);
            forward.addNodeDistance(nodeDist);
            
            nodeDistances.insert(nodeDist);
            nodePaths.insert(std::make_pair(nodeDistance.first, connection));
            
            // notify node owner of new node accessible.
            if(notifyNewNodeStatusCallback)
            {
                ioservice.post(std::bind(notifyNewNodeStatusCallback, nodeDistance.first, true));
            }
        }
        else if (it->second > nodeDistance.second + 1)
        {
            auto nodeDist = std::make_pair(nodeDistance.first, nodeDistance.second + 1);
            forward.addNodeDistance(nodeDist);
            
            it->second = nodeDistance.second + 1;
            auto itNodePath = nodePaths.find(nodeDistance.first);
            itNodePath->second = connection;
        }
        else if (it->second + 1 < nodeDistance.second)
        {
            reply.addNodeDistance(*it);
        }
    }
}

void Node::processFailedNodes(
        RoutingMessage &message,
        RoutingMessage &reply,
        RoutingMessage &forward,
        SharedConnection connection)
{
    for (auto node : message.getFailedNodes())
    {
        auto it = nodePaths.find(node);
        if (it != nodePaths.end())
        {
            if (it->second == connection)
            {
                nodeDistances.erase(node);
                nodePaths.erase(node);
                forward.addFailedNode(node);
                if(loggerDistance.first == node)
                {
                    loggerDistance = std::make_pair("", 0);
                }
                
                // notify node owner of failed node
                if(notifyNewNodeStatusCallback)
                {
                    notifyNewNodeStatusCallback(node, false);
                }
            }
            else
            {
                auto iterator = nodeDistances.find(node);
                reply.addNodeDistance(*iterator);
            }
        }
    }
}

void Node::processLogger(
        RoutingMessage &message,
        RoutingMessage &reply,
        RoutingMessage &forward)
{
    auto messageLoggerDistance = message.getLoggerDistance();

    if (messageLoggerDistance.first != "")
    {
        if (
            loggerDistance.first == "" ||
            loggerDistance.second > messageLoggerDistance.second + 1)
        {
            loggerDistance = messageLoggerDistance;
            loggerDistance.second++;
            forward.addLogger(loggerDistance);
        }
        else if (loggerDistance.second < messageLoggerDistance.second + 1)
        {
            reply.addLogger(loggerDistance);
        }
    }
}

    void Node::startConnection(SharedConnection connection, Connection::Type type)
    {
        if (type == Connection::eConnected)
        {
            Handshake message(name);
            connection->send(message, [this, connection](boost::system::error_code ec) {
                if (!ec) {
                    // Wait for reply.
                    connection->read(
                            std::bind(
                                    &Node::onRead,
                                    this,
                                    std::placeholders::_1,
                                    std::placeholders::_2
                            )
                    );
                }
            });
        }
        else if (type == Connection::eAccepted)
        {
            // waiting for handshake message.
            connection->read(
                    std::bind(
                            &Node::onRead,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2
                    )
            );
        }
    }

    void Node::log(const std::string &logMessage)
    {
        if (loggerDistance.first != "")
        {
            auto millisecondsSinceEpoch = 
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count();
            LogMessage message(millisecondsSinceEpoch, name, loggerDistance.first, logMessage);

            auto connection = getConnectionToNode(loggerDistance.first);
            if (connection)
            {
                ioservice.post(
                    [this, connection, message]
                    {
                        connection->send(
                                message,
                                std::bind(
                                        &Node::onWrite,
                                        this,
                                        message,
                                        std::placeholders::_1
                                )
                        );
                    }
                );
            }
        }
    }

    SharedConnection Node::getConnectionToNode(const std::string &nodeName)
    {
        if (isNodeAccessible(nodeName))
        {
            auto namePath = nodePaths.find(nodeName);
            auto connection = namePath->second;
            return connection;
        }
        return nullptr;
    }
}
