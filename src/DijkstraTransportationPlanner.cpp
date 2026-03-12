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

    std::string GetStreetName(CStreetMap::TNodeID src,
                              CStreetMap::TNodeID dest) const {
        // loop through all Ways
        for (size_t i = 0; i < config->StreetMap()->WayCount(); ++i) {
            auto way = config->StreetMap()->WayByIndex(i);
            bool found_src = false;
            bool found_dest = false;

            // check whether src and dest contained in the way
            for (size_t j = 0; j < way->NodeCount(); ++j) {
                if (way->GetNodeID(j) == src) {
                    found_src = true;
                }
                if (way->GetNodeID(j) == dest) {
                    found_dest = true;
                }
            }

            // get way name if both src and dest found
            if (found_src && found_dest) {
                if (way->HasAttribute("name")) {
                    return way->GetAttribute("name");
                }
            }
        }
        return "";
    }

    bool GetPathDescription(const std::vector<TTripStep> &path,
                            std::vector<std::string> &desc) const {
        // if no path found, return false
        if (path.empty()) {
            return false;
        }
        desc.clear();

        // if path found, covert start node location to DMS format, store in
        // description
        TNodeID start_node_id = path[0].second;
        std::shared_ptr<CStreetMap::SNode> start_node_obj =
            config->StreetMap()->NodeByID(start_node_id);
        CStreetMap::SLocation start_loc = start_node_obj->Location();
        std::string formated_start_loc =
            SGeographicUtils::ConvertLLToDMS(start_loc);
        desc.push_back("Start at " + formated_start_loc);

        // if only have start, end early
        if (path.size() == 1) {
            desc.push_back("End at " + formated_start_loc);
            return true;
        }

        // store info
        double cur_dist = 0.0;
        std::string cur_street = "";
        std::string cur_dir = "";
        auto cur_mode = path[0].first;

        // loop through path
        for (size_t i = 0; i < path.size() - 1; i++) {
            CStreetMap::TNodeID curID = path[i].second;
            CStreetMap::TNodeID nextID = path[i + 1].second;
            auto mode = path[i].first;

            // bus
            if (mode == CTransportationPlanner::ETransportationMode::Bus) {
                // find start and end stop
                size_t j = i;
                while (j < path.size() - 1 &&
                       path[j].first ==
                           CTransportationPlanner::ETransportationMode::Bus) {
                    j++;
                }
                auto start_stop_ID =
                    busSystemIndexer.StopByNodeID(path[i].second);
                auto end_stop_ID =
                    busSystemIndexer.StopByNodeID(path[j].second);

                // get route name
                std::unordered_set<std::shared_ptr<CBusSystem::SRoute>> routes;
                busSystemIndexer.RoutesByNodeIDs(path[i].second,
                                                 path[i + 1].second, routes);
                std::string route_name = "Unknown";
                if (!routes.empty()) {
                    route_name = (*routes.begin())->Name();
                }

                // continue the loop with did not get start or end stop ID
                if (!start_stop_ID || !end_stop_ID) {
                    continue;
                }

                std::string bus_result = "Take Bus " + route_name +
                                         " from stop " +
                                         std::to_string(start_stop_ID->ID());
                bus_result = bus_result + " to stop " +
                             std::to_string(end_stop_ID->ID());
                desc.push_back(bus_result);

                // update i
                i = j - 1;
                continue;
            }

            // walk/bike
            // get two location
            auto location_1 = config->StreetMap()->NodeByID(curID)->Location();
            auto location_2 = config->StreetMap()->NodeByID(nextID)->Location();

            // get distance, angle, direction
            double step_distance = SGeographicUtils::HaversineDistanceInMiles(
                location_1, location_2);
            double step_bearing =
                SGeographicUtils::CalculateBearing(location_1, location_2);
            std::string step_direction =
                SGeographicUtils::BearingToDirection(step_bearing);

            // get street name
            std::string step_street_name = GetStreetName(curID, nextID);

            // determine whether make change
            bool is_not_first_step = (i > 0);
            bool mode_is_changed = (mode != cur_mode);
            bool street_is_changed = (step_street_name != cur_street);
            bool direction_is_changed = (step_direction != cur_dir);

            // if mode / street name / direction change, add one description
            if (is_not_first_step && (mode_is_changed || street_is_changed ||
                                      direction_is_changed)) {
                std::string mode_string;
                if (cur_mode ==
                    CTransportationPlanner::ETransportationMode::Bike) {
                    mode_string = "Bike ";
                } else {
                    mode_string = "Walk ";
                }

                std::string connect_word;
                std::string target_name;

                if (!cur_street.empty()) {
                    // 'along' if have stree name
                    connect_word = " along ";
                    target_name = cur_street;
                } else {
                    //'toward' if dont have street name
                    connect_word = " toward ";
                    if (!step_street_name.empty()) {
                        target_name = step_street_name;
                    } else {
                        target_name = "End";
                    }
                }

                // format distance number
                char distance_buffer[64];
                sprintf(distance_buffer, "%.1f", cur_dist);

                // format description
                std::string walk_bike_result =
                    mode_string + cur_dir + connect_word + target_name +
                    " for " + distance_buffer + " mi";
                desc.push_back(walk_bike_result);

                cur_dist = 0.0;
            }

            // update variable
            cur_dist = cur_dist + step_distance;
            cur_mode = mode;
            cur_street = step_street_name;
            cur_dir = step_direction;
        }

        // last description
        if (cur_dist > 0.0) {
            // mode
            std::string final_mode_name;
            if (cur_mode == CTransportationPlanner::ETransportationMode::Bike) {
                final_mode_name = "Bike ";
            } else {
                final_mode_name = "Walk ";
            }

            // along / toward
            std::string connect_word;
            std::string final_place_name;

            if (!cur_street.empty()) {
                // if street have name, along
                connect_word = " along ";
                final_place_name = cur_street;
            } else {
                // if not, toward
                connect_word = " toward ";
                final_place_name = "End";
            }

            // format distance
            char final_dist_buffer[64];
            sprintf(final_dist_buffer, "%.1f", cur_dist);

            // final description
            std::string final_result = final_mode_name + cur_dir +
                                       connect_word + final_place_name +
                                       " for " + final_dist_buffer + " mi";
            desc.push_back(final_result);
        }

        // dest
        // get last node id, object, return true when there is dest node obj
        TNodeID last_node_id = path.back().second;
        std::shared_ptr<CStreetMap::SNode> last_node_object =
            config->StreetMap()->NodeByID(last_node_id);
        if (last_node_object != nullptr) {
            std::string final_location_str =
                SGeographicUtils::ConvertLLToDMS(last_node_object->Location());
            desc.push_back("End at " + final_location_str);
        }

        return true;
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
