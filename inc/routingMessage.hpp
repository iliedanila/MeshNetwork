#ifndef routingMessage_hpp
#define routingMessage_hpp

#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <string>
#include <vector>

namespace NetworkLayer {

// A route as its sender offers it: the hops from the sender (first) to the
// destination (last). A receiver ignores any route that already contains it,
// which is what keeps routes loop-free.
struct RouteAdvert {
    std::vector<std::string> path;
    bool isLogger = false;

    const std::string& destination() const { return path.back(); }

    template <class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/) {
        ar & path;
        ar & isLogger;
    }
};

class RoutingMessage {
   public:
    const std::vector<RouteAdvert>& getAdverts() const { return adverts; }

    const std::vector<std::string>& getFailedNodes() const {
        return failedNodes;
    }

    void addAdvert(RouteAdvert advert) { adverts.push_back(std::move(advert)); }

    void addFailedNode(std::string node) {
        failedNodes.push_back(std::move(node));
    }

    bool empty() const { return adverts.empty() && failedNodes.empty(); }

   private:
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/) {
        ar & adverts;
        ar & failedNodes;
    }

    std::vector<RouteAdvert> adverts;
    std::vector<std::string> failedNodes;
};

}  // namespace NetworkLayer

#endif /* routingMessage_hpp */
