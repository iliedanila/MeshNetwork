#ifndef dataMessage_h
#define dataMessage_h

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

namespace NetworkLayer
{

class DataMessage
{
public:
    DataMessage()
    :
        distance(0),
        messageID(0)
    {}

    ~DataMessage() {}
    
    DataMessage(std::string source, std::string destination, std::string buff, uint32_t _messageID)
    :
        sourceNodeName(std::move(source)),
        destinationNodeName(std::move(destination)),
        buffer(std::move(buff)),
        distance(0),
        messageID(_messageID)
    {}

    std::string Source() const { return sourceNodeName; }
    std::string Buffer() const { return buffer; }
    uint32_t Distance() const { return distance; }
    uint32_t MessageID() const { return messageID; }

private:
    friend class boost::serialization::access;
    friend class Node;
    
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & sourceNodeName;
        ar & destinationNodeName;
        ar & buffer;
        ar & distance;
        ar & messageID;
    }

private:
    std::string sourceNodeName;
    std::string destinationNodeName;
    std::string buffer;
    uint32_t distance;
    uint32_t messageID;
};

}

#endif /* dataMessage_h */
