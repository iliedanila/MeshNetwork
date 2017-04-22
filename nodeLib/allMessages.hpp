#ifndef allMessages_h
#define allMessages_h

#include <boost/variant.hpp>
#include "routingMessage.hpp"
#include "dataMessage.hpp"
#include "dataMessageAck.hpp"
#include "logMessage.hpp"

namespace NetworkLayer
{

typedef boost::variant<
    RoutingMessage,
    DataMessage,
    DataMessageAck,
    LogMessage> MessageVariant;
    
}

#endif /* allMessages_h */
