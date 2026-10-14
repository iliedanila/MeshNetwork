#ifndef node_hpp
#define node_hpp

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <functional>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

#include "allMessages.hpp"
#include "connection.hpp"
#include "sendError.hpp"

namespace NetworkLayer {

typedef std::function<void(std::string, SendError)> AckMessageCallback;
typedef std::function<void(std::string, bool)> NewNodeStatusCallbackT;

class Node {
   public:
    Node(std::string _name, boost::asio::io_context& _io_context,
         bool _isLogger = false);
    ~Node();

    void accept(unsigned short _port);
    void connect(std::string _host, unsigned short _port,
                 bool _reconnect = false);

    std::string& getName() { return name; }

    void close();

    std::vector<std::string> getAccessibleNodes();

    bool isNodeAccessible(const std::string& nodeName);

    void notifyNewNodeStatus(NewNodeStatusCallbackT callback);

    void sndMessage(std::string destination, std::string data,
                    std::function<void(std::string, SendError)> callback);

    void acceptMessages(std::function<void(DataMessage&)> callback);

    void log(const std::string& logMessage);

    boost::asio::io_context& getIOService() const { return ioservice; }

   private:
    SharedConnection addConnection(
        boost::asio::ip::tcp::socket&& socket,
        std::optional<ReconnectTarget> reconnectTarget);

    void startConnection(SharedConnection connection, Connection::Type type);

    void closeConnection(SharedConnection);

    void sendRoutingToNewConnection(SharedConnection);

    void onRead(MessageVariant, SharedConnection);

    void onWrite(MessageVariant message, boost::system::error_code error) const;

    void onConnectFailed(const std::string& host, unsigned short port,
                         bool reconnect,
                         const boost::system::error_code& error);

    void onReconnectTimer(std::shared_ptr<boost::asio::steady_timer> timer,
                          std::string host, unsigned short port,
                          const boost::system::error_code& error);

    // One handler per MessageVariant alternative. onRead dispatches with
    // std::visit, so a missing one is a compile error.
    void handleMessage(RoutingMessage&, SharedConnection);
    void handleMessage(DataMessage&, SharedConnection);
    void handleMessage(DataMessageAck&, SharedConnection);
    void handleMessage(LogMessage&, SharedConnection);
    void handleMessage(Handshake&, SharedConnection);
    void handleMessage(HandshakeReply&, SharedConnection);

    // A route as this node holds it: the hops from the next hop (first) to
    // the destination (last), and the connection to the next hop.
    struct Route {
        std::vector<std::string> path;
        SharedConnection via;
        bool isLogger;
    };

    void processAdverts(const RoutingMessage& message, RoutingMessage& reply,
                        RoutingMessage& forward, SharedConnection connection);

    void processFailedNodes(const RoutingMessage& message,
                            RoutingMessage& reply, RoutingMessage& forward,
                            SharedConnection connection);

    void withdrawRoute(const std::string& destination, RoutingMessage& forward);

    RouteAdvert advertise(const Route& route) const;

    const std::string* nearestLogger() const;

    void completePendingSend(uint64_t messageID, SendError error);

    void failPendingSends(const std::string& destination);

    SharedConnection getConnectionToNode(const std::string& nodeName);

    std::string name;
    std::set<SharedConnection> connections;
    std::map<std::string, Route> routes;

    struct PendingSend {
        std::string destination;
        AckMessageCallback callback;
    };

    std::unordered_map<uint64_t, PendingSend> pendingSends;
    uint64_t lastMessageID = 0;

    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor;

    bool closing;
    std::function<void(DataMessage&)> messageAcceptor;
    NewNodeStatusCallbackT notifyNewNodeStatusCallback;

    boost::asio::ip::tcp::socket accept_socket;
    boost::asio::io_context& ioservice;
    bool isLogger;
};

}  // namespace NetworkLayer

#endif /* node_hpp */
