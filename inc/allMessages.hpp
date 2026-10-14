#ifndef allMessages_h
#define allMessages_h

#include <variant>

#include "dataMessage.hpp"
#include "dataMessageAck.hpp"
#include "handshake.hpp"
#include "handshakeReply.hpp"
#include "logMessage.hpp"
#include "routingMessage.hpp"

namespace NetworkLayer {

// A frame carries the index of its alternative, so new alternatives go at
// the end; reordering or inserting breaks running nodes.
using MessageVariant = std::variant<RoutingMessage, DataMessage, DataMessageAck,
                                    LogMessage, Handshake, HandshakeReply>;
}  // namespace NetworkLayer

#endif /* allMessages_h */
