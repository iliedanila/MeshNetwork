#ifndef connection_hpp
#define connection_hpp

#include <boost/asio.hpp>
#include <memory>
#include <deque>

#include "allMessages.hpp"
#include "message.hpp"

using namespace boost::asio;
using namespace ip;

namespace NetworkLayer
{

class Node;
class Connection;
typedef std::shared_ptr<Connection> SharedConnection;
typedef std::function<void(boost::system::error_code)> WriteCallback;
typedef std::function<void(MessageVariant, SharedConnection)> ReadCallback;
typedef std::deque<std::pair<Message, WriteCallback>> MessageQueue;

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    enum Type
    {
        eAccepted,
        eConnected
    };

    Connection(
        Node& node,
        io_service& ioservice,
        tcp::socket&& socket,
        bool reconnect,
        std::function<void(std::shared_ptr<Connection>)> closeHandler);

    ~Connection();
    
    void Read(ReadCallback _callback);
    
    void Send(MessageVariant, WriteCallback);

    void Close();

    std::string RemoteIP() const { return remoteIP; }
    unsigned short RemotePort() const { return remotePort; }

    bool ReconnectIfClosed() const { return reconnect; }
    
private:
    void Write();

    void ReadHeader(ReadCallback _callback);
    void ReadBody(ReadCallback _callback);

    io_service& ioservice;
    tcp::socket socket;
    bool reconnect;
    std::function<void(std::shared_ptr<Connection>)> closeHandler;

    std::string remoteIP;
    unsigned short remotePort;
    
    Node& node;
    Message readMessage;
    MessageQueue writeMessages;
};
    
}

#endif /* connection_hpp */
