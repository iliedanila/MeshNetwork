#include "connection.hpp"
#include "allMessages.hpp"
#include "node.hpp"

#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/variant.hpp>

namespace NetworkLayer
{

Connection::Connection(
    Node& _node,
    boost::asio::io_service& _ioservice,
    tcp::socket&& _socket,
    bool _reconnect,
    std::function<void(std::shared_ptr<Connection>)> _closeHandler)
:
    ioservice(_ioservice),
    socket(std::move(_socket)),
    reconnect(_reconnect),
    closeHandler(_closeHandler),
    remoteIP(socket.remote_endpoint().address().to_string()),
    remotePort(socket.remote_endpoint().port()),
    node(_node)
{
}

Connection::~Connection()
{
}

void Connection::write()
{
    auto self(shared_from_this());

    boost::asio::async_write(
        socket,
        buffer(
                writeMessages.front().first.getOutputBuffer(),
            writeMessages.front().first.getOutputBuffer().size()
        ),
        [this, self]
        (boost::system::error_code error, uint32_t /*lenght*/)
        {
            if(!error)
            {
                // Callback.
                writeMessages.front().second(error);

                writeMessages.pop_front();
                if(!writeMessages.empty())
                {
                    write();
                }
            }
        }
    );
}

void Connection::read(ReadCallback _callback)
{
    readHeader(_callback);
}

void Connection::send(MessageVariant _message, WriteCallback _callback)
{
    std::stringstream ss;
    boost::archive::text_oarchive oarchive(ss);
    oarchive << _message;

    Message message(ss.str());
    message.createOutputBuffer();

    auto writeInProgress = !writeMessages.empty();
    writeMessages.push_back(std::make_pair(message, _callback));

    if (!writeInProgress)
    {
        write();
    }
}

void Connection::close()
{
    socket.close();
}

void Connection::readHeader(ReadCallback _callback)
{
    auto self(shared_from_this());

    boost::asio::async_read(
        socket,
        buffer(readMessage.getHeader(), Message::eHeaderLength),
        [this, self, _callback]
        (boost::system::error_code error_code, uint32_t length)
        {
            if(!error_code)
            {
                readMessage.decodeHeader();
                readBody(_callback);
            }
            else
            {
                std::cout << "Error reading header: " << error_code.message() << "\n";
                if (closeHandler)
                {
                    closeHandler(self);
                }
            }
        }
    );
}

void Connection::readBody(ReadCallback _callback)
{
    auto self(shared_from_this());

    boost::asio::async_read(
        socket,
        buffer(readMessage.getBody(), readMessage.getBodySize()),
        [this, self, _callback]
        (boost::system::error_code error_code, uint32_t /*length*/)
        {
            if (!error_code)
            {
                std::string content(readMessage.getBody().begin(), readMessage.getBody().end());
                std::stringstream ss(content);
                boost::archive::text_iarchive iarchive(ss);

                MessageVariant message;
                iarchive >> message;

                _callback(message, self);
                readHeader(_callback);
            }
            else
            {
                std::cout << "Error reading body: " << error_code.message() << "\n";
                if (closeHandler)
                {
                    closeHandler(self);
                }
            }
        }
    );
}

}
