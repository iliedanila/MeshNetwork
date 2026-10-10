#include "connection.hpp"
#include "allMessages.hpp"
#include "node.hpp"

// #include <boost/archive/text_iarchive.hpp>
// #include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/variant.hpp>
#include <iostream>

namespace NetworkLayer {

Connection::Connection(
    Node& _node, boost::asio::io_context& _ioservice, tcp::socket&& _socket,
    bool _reconnect,
    std::function<void(std::shared_ptr<Connection>)> _closeHandler)
    : ioservice(_ioservice),
      socket(std::move(_socket)),
      reconnect(_reconnect),
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

void Connection::write() {
    auto self(shared_from_this());

    boost::asio::async_write(
        socket,
        buffer(writeMessages.front().first.getOutputBuffer(),
               writeMessages.front().first.getOutputBuffer().size()),
        [this, self](boost::system::error_code error, uint32_t /*lenght*/) {
            if (error) {
                failQueuedWrites(error);
                closeAfterError();
                return;
            }
            writeMessages.front().second(error);

            writeMessages.pop_front();
            if (!writeMessages.empty()) {
                write();
            }
        });
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
    readHeader(_callback);
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
        write();
    }
}

void Connection::close() {
    boost::system::error_code ignored;
    socket.close(ignored);
}

void Connection::readHeader(ReadCallback _callback) {
    auto self(shared_from_this());

    boost::asio::async_read(
        socket, buffer(readMessage.getHeader(), Message::eHeaderLength),
        [this, self, _callback](boost::system::error_code error_code,
                                uint32_t length) {
            if (error_code) {
                std::cout << "Error reading header: " << error_code.message()
                          << "\n";
                closeAfterError();
            } else if (!readMessage.decodeHeader()) {
                node.log("Closing connection to " + remoteIP +
                         ": frame exceeds the size limit");
                closeAfterError();
            } else {
                readBody(_callback);
            }
        });
}

void Connection::readBody(ReadCallback _callback) {
    auto self(shared_from_this());

    boost::asio::async_read(
        socket, buffer(readMessage.getBody(), readMessage.getBodySize()),
        [this, self, _callback](boost::system::error_code error_code,
                                uint32_t /*length*/) {
            if (error_code) {
                std::cout << "Error reading body: " << error_code.message()
                          << "\n";
                closeAfterError();
                return;
            }

            MessageVariant message;
            try {
                std::string content(readMessage.getBody().begin(),
                                    readMessage.getBody().end());
                std::stringstream ss(content);
                boost::archive::binary_iarchive iarchive(ss);
                iarchive >> message;
            } catch (const std::exception& error) {
                node.log("Closing connection to " + remoteIP +
                         ": undecodable frame (" + error.what() + ")");
                closeAfterError();
                return;
            }

            _callback(message, self);
            readHeader(_callback);
        });
}

}  // namespace NetworkLayer
