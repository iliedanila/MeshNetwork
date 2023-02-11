#ifndef _LOG_MESSAGE_HPP_
#define _LOG_MESSAGE_HPP_

#include <boost/serialization/access.hpp>
#include <cstdint>
#include <string>

namespace NetworkLayer {
class LogMessage {
   public:
    LogMessage() : millisecondsSinceEpoch(0) {}

    ~LogMessage() {}

    explicit LogMessage(int64_t _millisecondsSinceEpoch,
                        const std::string& _sourceNodeName,
                        const std::string& _destinationNodeName,
                        const std::string& _logMessage)
        : millisecondsSinceEpoch(_millisecondsSinceEpoch),
          sourceNodeName(_sourceNodeName),
          destinationNodeName(_destinationNodeName),
          logMessage(_logMessage) {}

    int64_t getMillisecondsSinceEpoch() const { return millisecondsSinceEpoch; }

    const std::string& getSourceNodeName() const { return sourceNodeName; }

    const std::string& getDestinationNodeName() const {
        return destinationNodeName;
    }

    const std::string& getLogMessage() const { return logMessage; }

   private:
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar& millisecondsSinceEpoch;
        ar& sourceNodeName;
        ar& destinationNodeName;
        ar& logMessage;
    }

    int64_t millisecondsSinceEpoch;
    std::string sourceNodeName;
    std::string destinationNodeName;
    std::string logMessage;
};
}  // namespace NetworkLayer

#endif