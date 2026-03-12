#include "DijkstraTransportationPlanner.h"

#include "BusSystemIndexer.h"
#include "DijkstraPathRouter.h"
#include "DijkstraPathRouter_p.h"
#include "GeographicUtils.h"
#include "OpenStreetMap.h"
#include "TransportationPlanner.h"
#include <algorithm>
#include <memory>
#include <queue>
#include <unordered_map>

struct CDijkstraTransportationPlanner::SImplementation {
    std::shared_ptr<SConfiguration> config;

    CBusSystemIndexer busSystemIndexer;

    CDijkstraPathRouter shortest_router;
    CDijkstraPathRouter walk_bus_router;
    CDijkstraPathRouter walk_bike_router;

    struct SEdgeInfo {
        bool canBike;
        double speedLimit;
    };

    struct SPairHash {
        std::size_t operator()(const std::pair<TNodeID, TNodeID> &pair) const {
            return std::hash<TNodeID>{}(pair.first) ^
                   (std::hash<TNodeID>{}(pair.second) << 1);
        }
    };

    std::unordered_map<std::pair<TNodeID, TNodeID>, SEdgeInfo, SPairHash>
        edgeInfoMap;

    explicit SImplementation(const std::shared_ptr<SConfiguration> &config)
        : busSystemIndexer(config->BusSystem()) {
        this->config = config;

        const auto streetMap = config->StreetMap();

        for (std::size_t index = 0; index < streetMap->NodeCount(); ++index) {
            // trick: I add check so that when we promise the path router
            // our vertex tag is a unique id, it uses the unique id that we
            // pass in as vertex id, so that we don't have to store a
            // mapping between nodeId and vertexId.
            auto node = std::make_any<SUniqueIdWithData>(
                streetMap->NodeByIndex(index)->ID());

            shortest_router.AddVertex(node);
            walk_bus_router.AddVertex(node);
            walk_bike_router.AddVertex(std::move(node));
        }

        for (std::size_t way_index = 0; way_index < streetMap->WayCount();
             ++way_index) {
            auto way = streetMap->WayByIndex(way_index);

            const auto oneway = way->HasAttribute("oneway")
                                    ? way->GetAttribute("oneway") == "yes"
                                    : false;
            const bool bicycle = way->HasAttribute("bicycle")
                                     ? way->GetAttribute("bicycle") == "yes"
                                     : false;

            double speedLimit = config->DefaultSpeedLimit();
            if (way->HasAttribute("maxspeed")) {
                std::string speedStr = way->GetAttribute("maxspeed");
                if (speedStr.find("mph") != std::string::npos) {
                    speedStr = speedStr.substr(0, speedStr.find("mph"));
                }
                try {
                    speedLimit = std::stod(speedStr);
                } catch (...) {
                    speedLimit = config->DefaultSpeedLimit();
                }
            }

            for (std::size_t node_index = 1; node_index < way->NodeCount();
                 ++node_index) {
                const auto node1 =
                    streetMap->NodeByID(way->GetNodeID(node_index - 1));
                const auto node2 =
                    streetMap->NodeByID(way->GetNodeID(node_index));

                const double length =
                    SGeographicUtils::HaversineDistanceInMiles(
                        node1->Location(), node2->Location());

                // shortest paths ignore bicycle property, but will not walk
                // back
                shortest_router.AddEdge(way->GetNodeID(0),
                                        way->GetNodeID(way->NodeCount() - 1),
                                        length, !oneway);

                // add walking time. Walking is always bidir for fastest path
                walk_bus_router.AddEdge(way->GetNodeID(0),
                                        way->GetNodeID(way->NodeCount() - 1),
                                        length / config->WalkSpeed(), true);
                walk_bike_router.AddEdge(way->GetNodeID(0),
                                         way->GetNodeID(way->NodeCount() - 1),
                                         length / config->WalkSpeed(), true);

                // add bicycling time
                if (bicycle) {
                    walk_bike_router.AddEdge(
                        way->GetNodeID(0), way->GetNodeID(way->NodeCount() - 1),
                        length / config->BikeSpeed(), !oneway);
                }

                // store edge info for quick lookup
                TNodeID fromNode = way->GetNodeID(node_index - 1);
                TNodeID toNode = way->GetNodeID(node_index);
                edgeInfoMap[{fromNode, toNode}] = {bicycle, speedLimit};
            }
        }

        // iterate through all the routes for walk_bus_router
        for (std::size_t route_idx = 0;
             route_idx < config->BusSystem()->RouteCount(); ++route_idx) {
            const auto route = config->BusSystem()->RouteByIndex(route_idx);

            // iterate though all the stops in the route
            for (std::size_t stop_idx = 0; stop_idx + 1 < route->StopCount();
                 ++stop_idx) {
                const auto srcStop =
                    config->BusSystem()->StopByID(route->GetStopID(stop_idx));
                const auto destStop = config->BusSystem()->StopByID(
                    route->GetStopID(stop_idx + 1));

                const auto srcNodeID = srcStop->NodeID();
                const auto destNodeID = destStop->NodeID();

                std::vector<TNodeID> path;
                const auto distance = shortest_router.FindShortestPath(
                    srcNodeID, destNodeID, path);
                if (distance == CPathRouter::NoPathExists) {
                    continue;
                }

                double totalTime = 0.0;
                for (std::size_t path_idx = 0; path_idx + 1 < path.size();
                     ++path_idx) {
                    TNodeID fromNode = path[path_idx];
                    TNodeID toNode = path[path_idx + 1];

                    auto it = edgeInfoMap.find({fromNode, toNode});
                    double segmentSpeed = config->DefaultSpeedLimit();
                    if (it != edgeInfoMap.end()) {
                        segmentSpeed = it->second.speedLimit;
                    }

                    totalTime += distance / segmentSpeed;
                }

                walk_bus_router.AddEdge(srcNodeID, destNodeID, totalTime,
                                        false);
            }
        }
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

    double FindShortestPath(const TNodeID src, const TNodeID dest,
                            std::vector<TNodeID> &path) {
        if (src == dest) {
            path.clear();
            path.push_back(src);
            return 0.0;
        }

        // we just pass in the TNodeID because shortest_router transparently
        // uses the nodeId as vertexId when we promise them they are unique
        return shortest_router.FindShortestPath(src, dest, path);
    }

    double FindFastestPath(const TNodeID src, const TNodeID dest,
                           std::vector<TTripStep> &path) {
        path.clear();

        if (src == dest) {
            path.emplace_back(ETransportationMode::Walk, src);
            return 0.0;
        }

        std::vector<TNodeID> bikePath;
        auto bikeTime = walk_bike_router.FindShortestPath(src, dest, bikePath);

        std::vector<TNodeID> busPath;
        auto busTime = walk_bus_router.FindShortestPath(src, dest, busPath);

        std::vector<TNodeID> *chosenPath = nullptr;
        bool isBikePath = false;
        double chosenTime = 0.0;

        if (bikeTime == CPathRouter::NoPathExists &&
            busTime == CPathRouter::NoPathExists) {
            return CPathRouter::NoPathExists;
        }
        if (bikeTime == CPathRouter::NoPathExists || bikeTime >= busTime) {
            chosenPath = &busPath;
            isBikePath = false;
            chosenTime = busTime;
        } else {
            chosenPath = &bikePath;
            isBikePath = true;
            chosenTime = bikeTime;
        }

        const auto &nodePath = *chosenPath;
        for (std::size_t i = 0; i < nodePath.size(); ++i) {
            TNodeID currentNode = nodePath[i];
            ETransportationMode mode;

            if (i == 0) {
                TNodeID nextNode = nodePath[i + 1];
                if (isBikePath) {
                    mode = GetBikeModeForEdge(currentNode, nextNode);
                } else {
                    mode = GetBusModeForEdge(currentNode, nextNode);
                }
            } else {
                TNodeID prevNode = nodePath[i - 1];
                if (isBikePath) {
                    mode = GetBikeModeForEdge(prevNode, currentNode);
                } else {
                    mode = GetBusModeForEdge(prevNode, currentNode);
                }
            }

            path.emplace_back(mode, currentNode);
        }

        return chosenTime;
    }

    ETransportationMode GetBikeModeForEdge(TNodeID from, TNodeID to) const {
        auto it = edgeInfoMap.find({from, to});
        if (it != edgeInfoMap.end() && it->second.canBike) {
            return ETransportationMode::Bike;
        }
        return ETransportationMode::Walk;
    }

    ETransportationMode GetBusModeForEdge(TNodeID from, TNodeID to) const {
        if (busSystemIndexer.RouteBetweenNodeIDs(from, to)) {
            return ETransportationMode::Bus;
        }
        return ETransportationMode::Walk;
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

double
CDijkstraTransportationPlanner::FindFastestPath(TNodeID src, TNodeID dest,
                                                std::vector<TTripStep> &path) {
    return DImplementation->FindFastestPath(src, dest, path);
}

bool CDijkstraTransportationPlanner::GetPathDescription(
    const std::vector<TTripStep> &path, std::vector<std::string> &desc) const {
    return true;
}


