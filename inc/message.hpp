#ifndef _MESSAGE_HPP_
#define _MESSAGE_HPP_

#include <boost/asio/detail/buffer_sequence_adapter.hpp>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace NetworkLayer {
class Message {
   public:
    enum { eHeaderLength = 4 };

    Message() : bodySize(0) {}

    Message(const std::string& aBody) {
        bodySize = aBody.size();
        body.assign(aBody.begin(), aBody.end());
    }

    char* getHeader() { return header; }

    std::vector<char>& getBody() { return body; }

    std::size_t getBodySize() const { return bodySize; }

    void createOutputBuffer() {
        auto networkSize = htonl(body.size());
        memcpy(header, &networkSize, eHeaderLength);

        outputBuffer.clear();
        outputBuffer.insert(outputBuffer.end(), header, header + eHeaderLength);
        outputBuffer.insert(outputBuffer.end(), body.begin(), body.end());
    }

    std::vector<char>& getOutputBuffer() { return outputBuffer; }

    void decodeHeader() {
        uint32_t networkSize;
        memcpy(&networkSize, header, eHeaderLength);
        bodySize = ntohl(networkSize);
        body.reserve(bodySize);
        body.assign(bodySize, '\0');
    }

   private:
    std::size_t bodySize;
    char header[eHeaderLength + 1];
    std::vector<char> body;
    std::vector<char> outputBuffer;
};

}  // namespace NetworkLayer

#endif  // _MESSAGE_HPP_
