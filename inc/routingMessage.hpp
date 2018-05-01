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

    std::map<std::string, uint32_t> getNodeDistances() { return nodeDistances; }
    std::vector<std::string> getFailedNodes() { return failedNodes; }
    std::pair<std::string, uint32_t> getLoggerDistance() { return loggerDistance; }
    
    void addNodeDistance(std::pair<std::string, uint32_t> _nodeDistance)
    {
        nodeDistances.insert(_nodeDistance);
    }
    
    void addFailedNode(std::string node)
    {
        failedNodes.push_back(node);
    }

    void addLogger(std::pair<std::string, uint32_t> _loggerDistance)
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
    std::map<std::string, uint32_t> nodeDistances;
    std::vector<std::string> failedNodes;
    std::pair<std::string, uint32_t> loggerDistance;
};
    
}

#endif /* routingMessage_hpp */
