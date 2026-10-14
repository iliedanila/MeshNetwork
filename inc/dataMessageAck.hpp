#ifndef dataMessageAck_hpp
#define dataMessageAck_hpp

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <cstdint>

#include "sendError.hpp"

namespace NetworkLayer {

class DataMessageAck {
   public:
    DataMessageAck() : error() {}

    ~DataMessageAck() {}

    explicit DataMessageAck(std::string source, std::string destination,
                            SendError _error, uint64_t _messageID)
        : sourceNodeName(std::move(source)),
          destinationNodeName(std::move(destination)),
          error(_error),
          messageID(_messageID) {}

   private:
    friend class boost::serialization::access;
    friend class Node;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/) {
        ar & sourceNodeName;
        ar & destinationNodeName;
        ar & error;
        ar & messageID;
    }

   private:
    std::string sourceNodeName;
    std::string destinationNodeName;
    SendError error;
    uint64_t messageID;
};

}  // namespace NetworkLayer

#endif /* dataMessageAck_hpp */
