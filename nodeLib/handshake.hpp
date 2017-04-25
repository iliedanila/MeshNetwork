#ifndef _HANDSHAKE_HPP_
#define _HANDSHAKE_HPP_

#include <string>
#include <boost/serialization/access.hpp>

namespace NetworkLayer
{
    class Handshake
    {
    public:
        Handshake() {}
        ~Handshake() {}

        explicit Handshake(const std::string& _nodeName)
        :
            nodeName(_nodeName)
        {}

        const std::string& NodeName() const { return nodeName; }

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
