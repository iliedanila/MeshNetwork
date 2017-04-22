#ifndef routingMessage_hpp
#define routingMessage_hpp

#include <vector>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/utility.hpp>

namespace NetworkLayer
{

class RoutingMessage
{
public:
    RoutingMessage() {}
    ~RoutingMessage() {}
    
    auto NodeDistances() { return nodeDistances; }
    auto FailedNodes() { return failedNodes; }
    auto LoggerDistance() { return loggerDistance; }
    
    void AddNodeDistance(std::pair<std::string, std::size_t> _nodeDistance)
    {
        nodeDistances.insert(_nodeDistance);
    }
    
    void AddFailedNode(std::string node)
    {
        failedNodes.push_back(node);
    }

    void AddLogger(std::pair<std::string, std::size_t> _loggerDistance)
    {
        loggerDistance = _loggerDistance;
    }
    
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & nodeDistances;
        ar & failedNodes;
        ar & loggerDistance;
    }
    
private:
    std::map<std::string, std::size_t> nodeDistances;
    std::vector<std::string> failedNodes;
    std::pair<std::string, std::size_t> loggerDistance;
};
    
}

#endif /* routingMessage_hpp */
