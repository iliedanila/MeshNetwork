#ifndef _HANDSHAKE_HPP_
#define _HANDSHAKE_HPP_

#include <boost/serialization/access.hpp>
#include <string>

namespace NetworkLayer {
class Handshake {
   public:
    Handshake() {}

    ~Handshake() {}

    explicit Handshake(const std::string& _nodeName) : nodeName(_nodeName) {}

    const std::string& getNodeName() const { return nodeName; }

   private:
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar& nodeName;
    }

    std::string nodeName;
};
}  // namespace NetworkLayer

#endif
