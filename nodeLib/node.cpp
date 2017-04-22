#include "node.hpp"
#include "connection.hpp"
#include "allMessages.hpp"
#include "messageVisitor.hpp"

#include <string>
#include <functional>
#include <iostream>
#include <chrono>
#include <iomanip>

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

void Node::Accept(unsigned short _port)
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
                auto connection = AddConnection(std::move(accept_socket));
                    
                StartConnection(connection);
            }
            else
            {
                std::cout << name << " error accepting: "
                << error.message() << "\n";
            }
            if(!closing)
            {
                Accept(_port);
            }
        });
}

void Node::Connect(
    std::string host,
    unsigned short port)
{
    tcp::resolver resolver(ioservice);
    auto endpoint = resolver.resolve({host, std::to_string(port)});
    
    boost::asio::async_connect(
        connect_socket,
        endpoint,
        [this]
        (const boost::system::error_code& error, tcp::resolver::iterator)
        {
            if (!error)
            {
                auto connection = AddConnection(std::move(connect_socket));

                StartConnection(connection);
            }
            else
            {
                std::cout << name << " error connecting: "
                << error.message() << "\n";
            }
        });
}

void Node::Close()
{
    closing = true;
    for (auto connection : connections)
    {
        connection->Close();
    }
    if (acceptor)
    {
        acceptor->cancel();
    }
}

std::vector<std::string> Node::GetAccessibleNodes()
{
    std::vector<std::string> nodeList;
    for ( auto keyValue : nodeDistances )
    {
        nodeList.push_back(keyValue.first);
    }
    return nodeList;
}

bool Node::IsNodeAccessible(const std::string& nodeName)
{
    return nodeDistances.find(nodeName) != nodeDistances.end();
}
    
void Node::NotifyNewNodeStatus(std::function<void (std::string, bool)> callback)
{
    notifyNewNodeStatusCallback = callback;
}

void Node::SndMessage(
    std::string destination,
    std::string data,
    std::function< void(std::string, SendError)> callback)
{
    if (IsNodeAccessible(destination))
    {
        auto namePath = nodePaths.find(destination);
        auto connection = namePath->second;
        
        auto messageID = std::hash<std::string>{}(data);
        DataMessage message(name, destination, data, messageID);
        
        connection->Send(
            message, 
            std::bind(
                &Node::OnWrite, 
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
    
void Node::AcceptMessages(std::function<void (DataMessage&)> callback)
{
    messageAcceptor = callback;
}

void Node::CloseConnection(SharedConnection connectionDown)
{
    RoutingMessage nodesDown;
    std::vector<std::string> toDelete;
    for (auto nodePath : nodePaths)
    {
        if (nodePath.second == connectionDown)
        {
            nodesDown.AddFailedNode(nodePath.first);
            toDelete.push_back(nodePath.first);
        }
    }
    
    for (auto key : toDelete)
    {
        nodePaths.erase(key);
        nodeDistances.erase(key);
    }
    
    connectionDown->Close();
    connections.erase(connectionDown);
    
    if(closing)
    {
        return;
    }
    
    for (auto connection : connections)
    {
        connection->Send(
            nodesDown, 
            std::bind(
                &Node::OnWrite, 
                this, 
                nodesDown,
                std::placeholders::_1
            )
        );
    }
}

void Node::SendRoutingToNewConnection(SharedConnection connection)
{
    RoutingMessage message;
    message.AddNodeDistance(std::make_pair(name, 0));
    
    for (auto nodeDistance : nodeDistances)
    {
        message.AddNodeDistance(nodeDistance);
    }

    if(isLogger)
    {
        message.AddLogger(std::make_pair(name, 0));
    }

    connection->Send(
        message, 
        std::bind(
            &Node::OnWrite,
            this,
            message,
            std::placeholders::_1
        )
    );
}

SharedConnection Node::AddConnection(tcp::socket&& socket)
{
    auto connection = std::make_shared<Connection>(
        *this,
        ioservice,
        std::move(socket),
        std::bind(
            &Node::CloseConnection,
            this,
            std::placeholders::_1));
    
    connections.insert(connection);
    return connection;
}

void Node::OnRead(MessageVariant _message, SharedConnection _connection)
{
    boost::apply_visitor(MessageVisitor(*this, _connection), _message);
}

void Node::OnWrite(MessageVariant message, boost::system::error_code error) const
{
}

template <>
void Node::HandleMessage(RoutingMessage& _message, SharedConnection _connection)
{
    RoutingMessage reply;
    RoutingMessage forward;
    
    ProcessAddNodePaths(_message, reply, forward, _connection);
    ProcessFailedNodes(_message, reply, forward, _connection);
    ProcessLogger(_message, reply, forward);


    
    if (reply.NodeDistances().size() > 0 ||
        reply.FailedNodes().size() > 0 ||
        reply.LoggerDistance().first != "")
    {
        _connection->Send(
            reply,
            std::bind(
                &Node::OnWrite,
                this,
                reply,
                std::placeholders::_1
            )
        );
    }
    
    if (forward.NodeDistances().size() > 0 ||
        forward.FailedNodes().size() > 0 ||
        forward.LoggerDistance().first != "")
    {
        for (auto conn : connections)
        {
            if (conn != _connection)
            {
                conn->Send(
                    forward,
                    std::bind(
                        &Node::OnWrite,
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
void Node::HandleMessage(DataMessage& _message, SharedConnection _connection)
{
    _message.distance++;

    if (_message.destinationNodeName == name)
    {
        if (messageAcceptor)
        {
            messageAcceptor(_message);
            DataMessageAck message(name, _message.sourceNodeName, eSuccess, _message.MessageID());
            _connection->Send(
                message,
                std::bind(
                    &Node::OnWrite,
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
                _message.MessageID()
            );
            _connection->Send(
                message,
                std::bind(
                    &Node::OnWrite,
                    this,
                    message,
                    std::placeholders::_1
                )
            );
        }
    }
    else
    {
        if (IsNodeAccessible(_message.destinationNodeName))
        {
            auto namePath = nodePaths.find(_message.destinationNodeName);
            auto connection = namePath->second;
            connection->Send(
                _message,
                std::bind(
                    &Node::OnWrite,
                    this,
                    _message,
                    std::placeholders::_1
                )
            );
        }
        else
        {
            DataMessageAck message(_message.sourceNodeName, name, eNoPath, _message.MessageID());
            _connection->Send(
                message,
                std::bind(
                    &Node::OnWrite,
                    this,
                    message,
                    std::placeholders::_1
                )
            );
        }
    }
}
    
template<>
void Node::HandleMessage(DataMessageAck& _message, SharedConnection _connection)
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
        if (IsNodeAccessible(_message.destinationNodeName))
        {
            auto namePath = nodePaths.find(_message.destinationNodeName);
            auto connection = namePath->second;
            connection->Send(
                _message,
                std::bind(
                    &Node::OnWrite,
                    this,
                    _message,
                    std::placeholders::_1
                )
            );
        }
    }
}

template<>
void Node::HandleMessage(LogMessage& _message, SharedConnection _connection)
{
    if (_message.DestinationNodeName() == name)
    {
        std::chrono::time_point<std::chrono::system_clock> epoch;
        std::chrono::milliseconds millisecondsSinceEpoch{ _message.MillisecondsSinceEpoch() };
        auto logTime = std::chrono::system_clock::to_time_t(epoch + millisecondsSinceEpoch);

        std::cout
            << "LOG -- "
            << std::put_time(std::localtime(&logTime), "%F %T")
            << " -- "
            << _message.SourceNodeName()
            << " -- "
            << _message.Log()
            << "\n";
    }
    else
    {
        auto forwardConnection = GetConnectionToNode(_message.DestinationNodeName());
        if (forwardConnection)
        {
            ioservice.post(
                [this, forwardConnection, _message]
                {
                    forwardConnection->Send(
                        _message,
                        std::bind(
                            &Node::OnWrite,
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

void Node::ProcessAddNodePaths(
    RoutingMessage& message,
    RoutingMessage& reply,
    RoutingMessage& forward,
    SharedConnection connection)
{
    for (auto nodeDistance : message.NodeDistances())
    {
        auto it = nodeDistances.find(nodeDistance.first);
        if (it == nodeDistances.end())
        {
            auto nodeDist = std::make_pair(nodeDistance.first, nodeDistance.second + 1);
            forward.AddNodeDistance(nodeDist);
            
            nodeDistances.insert(nodeDist);
            nodePaths.insert(std::make_pair(nodeDistance.first, connection));
            
            // notify node owner of new node accessible.
            if(notifyNewNodeStatusCallback)
            {
                notifyNewNodeStatusCallback(nodeDistance.first, true);
            }
        }
        else if (it->second > nodeDistance.second + 1)
        {
            auto nodeDist = std::make_pair(nodeDistance.first, nodeDistance.second + 1);
            forward.AddNodeDistance(nodeDist);
            
            it->second = nodeDistance.second + 1;
            auto itNodePath = nodePaths.find(nodeDistance.first);
            itNodePath->second = connection;
        }
        else if (it->second + 1 < nodeDistance.second)
        {
            reply.AddNodeDistance(*it);
        }
    }
}

void Node::ProcessFailedNodes(
    RoutingMessage& message,
    RoutingMessage& reply,
    RoutingMessage& forward,
    SharedConnection connection)
{
    for (auto node : message.FailedNodes())
    {
        auto it = nodePaths.find(node);
        if (it != nodePaths.end())
        {
            if (it->second == connection)
            {
                nodeDistances.erase(node);
                nodePaths.erase(node);
                forward.AddFailedNode(node);
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
                reply.AddNodeDistance(*iterator);
            }
        }
    }
}

void Node::ProcessLogger(
    RoutingMessage& message,
    RoutingMessage& reply,
    RoutingMessage& forward)
{
    auto messageLoggerDistance = message.LoggerDistance();

    if (messageLoggerDistance.first != "")
    {
        if (
            loggerDistance.first == "" ||
            loggerDistance.second > messageLoggerDistance.second + 1)
        {
            loggerDistance = messageLoggerDistance;
            loggerDistance.second++;
            forward.AddLogger(loggerDistance);
        }
        else if (loggerDistance.second < messageLoggerDistance.second + 1)
        {
            reply.AddLogger(loggerDistance);
        }
    }
}

    void Node::StartConnection(SharedConnection connection)
    {
        connection->Read(
            std::bind(
                &Node::OnRead,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            )
        );
        
        ioservice.post(
            [this, connection]
            {
                SendRoutingToNewConnection(connection);
            }
        );
    }

    void Node::Log(const std::string& logMessage)
    {
        if (loggerDistance.first != "")
        {
            auto millisecondsSinceEpoch = 
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count();
            LogMessage message(millisecondsSinceEpoch, name, loggerDistance.first, logMessage);

            auto connection = GetConnectionToNode(loggerDistance.first);
            if (connection)
            {
                ioservice.post(
                    [this, connection, message]
                    {
                        connection->Send(
                            message,
                            std::bind(
                                &Node::OnWrite,
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

    SharedConnection Node::GetConnectionToNode(const std::string& nodeName)
    {
        if (IsNodeAccessible(nodeName))
        {
            auto namePath = nodePaths.find(nodeName);
            auto connection = namePath->second;
            return connection;
        }
        return nullptr;
    }
}
