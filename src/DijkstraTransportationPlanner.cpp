#include "DijkstraTransportationPlanner.h"
#include "DijkstraPathRouter.h"
#include "OpenStreetMap.h"
#include "TransportationPlanner.h"
#include <algorithm>
#include <memory>
#include <queue>
#include <ranges>
#include <unordered_map>

struct CDijkstraTransportationPlanner::SImplementation {
    std::shared_ptr<SConfiguration> config;

    CDijkstraPathRouter shortest_router;
    CDijkstraPathRouter transportation_router;

    explicit SImplementation(const std::shared_ptr<SConfiguration> &config) {
        this->config = config;
    }

    std::shared_ptr<CStreetMap::SNode>
    SortedNodeByIndex(const std::size_t index) const {
        if (index >= config->StreetMap()->NodeCount())
            return nullptr;

        // a shortcut that I provided
        // COpenStreetMap stores node within a map
        const auto osm =
            std::dynamic_pointer_cast<COpenStreetMap>(config->StreetMap());
        if (osm != nullptr)
            return osm->SortedNodeByIndex(index);

        std::vector<TNodeID> nodeIds;
        const auto nodeCount = config->StreetMap()->NodeCount();
        nodeIds.reserve(nodeCount);
        for (size_t i = 0; i < nodeCount; ++i) {
            nodeIds.emplace_back(config->StreetMap()->NodeByIndex(i)->ID());
        }
        std::ranges::sort(nodeIds);
        return config->StreetMap()->NodeByIndex(nodeIds[index]);
    }

    std::size_t NodeCount() const noexcept {
        return config->StreetMap()->NodeCount();
    }

    double FindShortestPath(TNodeID src, TNodeID dest,
                            std::vector<TNodeID> &path) {
        if (src == dest) {
            path.clear();
            path.push_back(src);
            return 0.0;
        }

        return 0.0;
    }
};

CDijkstraTransportationPlanner::CDijkstraTransportationPlanner(
    const std::shared_ptr<SConfiguration> &config) {
    DImplementation = std::make_unique<SImplementation>(config);
}

CDijkstraTransportationPlanner::~CDijkstraTransportationPlanner() = default;
std::size_t CDijkstraTransportationPlanner::NodeCount() const noexcept {
    return DImplementation->NodeCount();
}
std::shared_ptr<CStreetMap::SNode>
CDijkstraTransportationPlanner::SortedNodeByIndex(
    const std::size_t index) const noexcept {
    return DImplementation->SortedNodeByIndex(index);
}
double
CDijkstraTransportationPlanner::FindShortestPath(TNodeID src, TNodeID dest,
                                                 std::vector<TNodeID> &path) {
    return DImplementation->FindShortestPath(src, dest, path);
}
