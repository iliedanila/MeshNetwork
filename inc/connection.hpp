#ifndef connection_hpp
#define connection_hpp

#include <boost/asio.hpp>
#include <deque>
#include <memory>

#include "allMessages.hpp"
#include "message.hpp"

using namespace boost::asio;
using namespace ip;

namespace NetworkLayer {

class Node;
class Connection;
typedef std::shared_ptr<Connection> SharedConnection;
typedef std::function<void(boost::system::error_code)> WriteCallback;
typedef std::function<void(MessageVariant, SharedConnection)> ReadCallback;
typedef std::deque<std::pair<Message, WriteCallback>> MessageQueue;

class Connection : public std::enable_shared_from_this<Connection> {
   public:
    enum Type { eAccepted, eConnected };

    Connection(Node& node, io_context& ioservice, tcp::socket&& socket,
               bool reconnect,
               std::function<void(std::shared_ptr<Connection>)> closeHandler);

    ~Connection();

    void read(ReadCallback _callback);

    void send(MessageVariant, WriteCallback);

    void close();

    std::string getRemoteIP() const { return remoteIP; }

    uint16_t getRemotePort() const { return remotePort; }

    bool getReconnect() const { return reconnect; }

   private:
    void write();

    void readHeader(ReadCallback readCallback);
    void readBody(ReadCallback readCallback);

    io_context& ioservice;
    tcp::socket socket;
    bool reconnect;
    std::function<void(std::shared_ptr<Connection>)> closeHandler;

    std::string remoteIP;
    uint16_t remotePort;

    Node& node;
    Message readMessage;
    MessageQueue writeMessages;
};

}  // namespace NetworkLayer

#endif /* connection_hpp */
