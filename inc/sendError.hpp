#ifndef nodeErrors_hpp
#define nodeErrors_hpp

namespace NetworkLayer {
enum SendError { eSuccess, eNoPath, eNodeNotAccepting, eMessageTooLarge };
}

#endif /* nodeErrors_hpp */
