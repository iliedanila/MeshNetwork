#ifndef connection_hpp
#define connection_hpp

#include <boost/asio.hpp>
#include <deque>
#include <memory>
#include <optional>
#include <string>

#include "allMessages.hpp"
#include "message.hpp"

namespace NetworkLayer {

class Node;
class Connection;
typedef std::shared_ptr<Connection> SharedConnection;
typedef std::function<void(boost::system::error_code)> WriteCallback;
typedef std::function<void(MessageVariant, SharedConnection)> ReadCallback;
typedef std::deque<std::pair<Message, WriteCallback>> MessageQueue;

// Where to reconnect after an outgoing connection drops.
struct ReconnectTarget {
    std::string host;
    unsigned short port;
};

class Connection : public std::enable_shared_from_this<Connection> {
   public:
    enum Type { eAccepted, eConnected };

    Connection(Node& node, boost::asio::io_context& ioservice,
               boost::asio::ip::tcp::socket&& socket,
               std::optional<ReconnectTarget> reconnectTarget,
               std::function<void(std::shared_ptr<Connection>)> closeHandler);

    ~Connection();

    void read(ReadCallback _callback);

    void send(MessageVariant, WriteCallback);

    void close();

    std::string getRemoteIP() const { return remoteIP; }

    uint16_t getRemotePort() const { return remotePort; }

    const std::optional<ReconnectTarget>& getReconnectTarget() const {
        return reconnectTarget;
    }

   private:
    void write();
    void failQueuedWrites(boost::system::error_code error);
    void closeAfterError();

    void readHeader(ReadCallback readCallback);
    void readBody(ReadCallback readCallback);

    boost::asio::io_context& ioservice;
    boost::asio::ip::tcp::socket socket;
    std::optional<ReconnectTarget> reconnectTarget;
    std::function<void(std::shared_ptr<Connection>)> closeHandler;

    std::string remoteIP;
    uint16_t remotePort;

    Node& node;
    Message readMessage;
    MessageQueue writeMessages;
    bool closedAfterError = false;
};

}  // namespace NetworkLayer

#endif /* connection_hpp */
