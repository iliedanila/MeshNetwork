#ifndef _HANDSHAKE_REPLY_HPP_
#define _HANDSHAKE_REPLY_HPP_

#include <string>
#include <boost/serialization/access.hpp>

namespace NetworkLayer
{
    class HandshakeReply
    {
    public:
        HandshakeReply() {}
        ~HandshakeReply() {}

        explicit HandshakeReply(const std::string& _nodeName)
        :
            nodeName(_nodeName)
        {}

        const std::string& getNodeName() const { return nodeName; }

    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & nodeName;
        }

        std::string nodeName;
    };
}

#endif
