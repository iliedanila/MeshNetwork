#include "connection.hpp"
#include "allMessages.hpp"
#include "coroutines.hpp"
#include "node.hpp"

// #include <boost/archive/text_iarchive.hpp>
// #include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
// Writes a std::variant as an int index and the value, as boost::variant was
// written. Not boost/serialization/std_variant.hpp: its index is a size_t,
// which would change the wire format.
#include <boost/serialization/variant.hpp>
#include <iostream>

using boost::asio::ip::tcp;

namespace NetworkLayer {

Connection::Connection(
    Node& _node, boost::asio::io_context& _ioservice, tcp::socket&& _socket,
    std::optional<ReconnectTarget> _reconnectTarget,
    std::function<void(std::shared_ptr<Connection>)> _closeHandler)
    : ioservice(_ioservice),
      socket(std::move(_socket)),
      reconnectTarget(std::move(_reconnectTarget)),
      closeHandler(_closeHandler),
      remotePort(0),
      node(_node) {
    // The peer may already be gone; that surfaces on the first read.
    boost::system::error_code error;
    auto endpoint = socket.remote_endpoint(error);
    if (!error) {
        remoteIP = endpoint.address().to_string();
        remotePort = endpoint.port();
    }
}

Connection::~Connection() {}

// Writes the queued frames in order. The queue is non-empty exactly while
// this runs, and a frame stays at the front until its callback returns, so a
// callback that sends again only queues.
boost::asio::awaitable<void> Connection::writeLoop(SharedConnection /*self*/) {
    boost::system::error_code error;
    while (!writeMessages.empty()) {
        co_await boost::asio::async_write(
            socket,
            boost::asio::buffer(writeMessages.front().first.getOutputBuffer()),
            nothrowAwaitable(error));
        if (error) {
            failQueuedWrites(error);
            closeAfterError();
            co_return;
        }
        writeMessages.front().second(error);
        writeMessages.pop_front();
    }
}

void Connection::failQueuedWrites(boost::system::error_code error) {
    MessageQueue failed;
    failed.swap(writeMessages);
    for (auto& queued : failed) {
        queued.second(error);
    }
}

// Hands the connection to the owner's close handler exactly once, whichever
// of the read or write paths notices the failure first.
void Connection::closeAfterError() {
    if (closedAfterError) {
        return;
    }
    closedAfterError = true;
    if (closeHandler) {
        closeHandler(shared_from_this());
    } else {
        close();
    }
}

void Connection::read(ReadCallback _callback) {
    boost::asio::co_spawn(ioservice,
                          readLoop(shared_from_this(), std::move(_callback)),
                          rethrowException);
}

void Connection::send(MessageVariant _message, WriteCallback _callback) {
    std::stringstream ss;
    boost::archive::binary_oarchive oarchive(ss);
    oarchive << _message;

    if (ss.str().size() > Message::eMaxBodyLength) {
        boost::asio::post(ioservice, [_callback] {
            _callback(boost::asio::error::message_size);
        });
        return;
    }

    Message message(ss.str());
    message.createOutputBuffer();

    auto writeInProgress = !writeMessages.empty();
    writeMessages.push_back(std::make_pair(message, _callback));

    if (!writeInProgress) {
        boost::asio::co_spawn(ioservice, writeLoop(shared_from_this()),
                              rethrowException);
    }
}

void Connection::close() {
    boost::system::error_code ignored;
    socket.close(ignored);
}

// Reads frames and hands each to `callback` until reading fails or a frame is
// too large or undecodable; then the connection goes to closeAfterError.
boost::asio::awaitable<void> Connection::readLoop(SharedConnection self,
                                                  ReadCallback callback) {
    boost::system::error_code error;
    for (;;) {
        co_await boost::asio::async_read(
            socket,
            boost::asio::buffer(readMessage.getHeader(),
                                Message::eHeaderLength),
            nothrowAwaitable(error));
        if (error) {
            std::cout << "Error reading header: " << error.message() << "\n";
            closeAfterError();
            co_return;
        }
        if (!readMessage.decodeHeader()) {
            node.log("Closing connection to " + remoteIP +
                     ": frame exceeds the size limit");
            closeAfterError();
            co_return;
        }

        co_await boost::asio::async_read(
            socket,
            boost::asio::buffer(readMessage.getBody(),
                                readMessage.getBodySize()),
            nothrowAwaitable(error));
        if (error) {
            std::cout << "Error reading body: " << error.message() << "\n";
            closeAfterError();
            co_return;
        }

        MessageVariant message;
        try {
            std::string content(readMessage.getBody().begin(),
                                readMessage.getBody().end());
            std::stringstream ss(content);
            boost::archive::binary_iarchive iarchive(ss);
            iarchive >> message;
        } catch (const std::exception& exception) {
            node.log("Closing connection to " + remoteIP +
                     ": undecodable frame (" + exception.what() + ")");
            closeAfterError();
            co_return;
        }

        callback(message, self);
    }
}

}  // namespace NetworkLayer
