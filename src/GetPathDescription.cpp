#include "DijkstraTransportationPlanner.h"
#include "GeographicUtils.h"
#include "BusSystemIndexer.h"
#include <cmath>
#include <cstdio>
#include <unordered_set>

struct CDijkstraTransportationPlanner::SImplementation {
    std::shared_ptr<CStreetMap> StreetMap;
    std::shared_ptr<CBusSystemIndexer> BusSystemIndex;
};


std::string GetStreetName(CStreetMap::TNodeID src, CStreetMap::TNodeID dest, const CStreetMap* map) {
    if (!map) return "";
    
    // loop through all Ways
    for (size_t i = 0; i < map->WayCount(); ++i) {
        auto way = map->WayByIndex(i);
        bool found_src = false;
        bool found_dest = false;
        
        // check whether src and dest contained in the way
        for (size_t j = 0; j < way->NodeCount(); ++j) {
            if (way->GetNodeID(j) == src){
                found_src = true;
            }
            if (way->GetNodeID(j) == dest){
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

bool CDijkstraTransportationPlanner::GetPathDescription(const std::vector<TTripStep> &path, std::vector<std::string> &desc) const {
    // if no path found, return false
    if (path.empty()) {
        return false;
    }
    desc.clear();

    // if path found, covert start node location to DMS format, store in description
    TNodeID start_node_id = path[0].second; 
    std::shared_ptr<CStreetMap::SNode> start_node_obj = DImplementation->StreetMap->NodeByID(start_node_id);
    CStreetMap::SLocation start_loc = start_node_obj->Location();
    std::string formated_start_loc = SGeographicUtils::ConvertLLToDMS(start_loc);
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
            while (j < path.size() - 1 && path[j].first == CTransportationPlanner::ETransportationMode::Bus) {
                j++;
            }
            auto start_stop_ID = DImplementation->BusSystemIndex->StopByNodeID(path[i].second);
            auto end_stop_ID = DImplementation->BusSystemIndex->StopByNodeID(path[j].second);
            
            //get route name
            std::unordered_set<std::shared_ptr<CBusSystem::SRoute>> routes;
            DImplementation->BusSystemIndex->RoutesByNodeIDs(path[i].second, path[i+1].second, routes);
            std::string route_name = "Unknown";
            if (!routes.empty()) {
                route_name = (*routes.begin())->Name();
            }

            // continue the loop with did not get start or end stop ID
            if (!start_stop_ID || !end_stop_ID){
                continue;
            }

            std::string bus_result = "Take Bus " + route_name + " from stop " + std::to_string(start_stop_ID->ID());
            bus_result = bus_result + " to stop " + std::to_string(end_stop_ID->ID());
            desc.push_back(bus_result);
            
            // update i
            i = j - 1; 
            continue;
        }

        // walk/bike
        // get two location
        auto location_1 = DImplementation->StreetMap->NodeByID(curID)->Location();
        auto location_2 = DImplementation->StreetMap->NodeByID(nextID)->Location();
        
        // get distance, angle, direction
        double step_distance = SGeographicUtils::HaversineDistanceInMiles(location_1, location_2);
        double step_bearing = SGeographicUtils::CalculateBearing(location_1, location_2);
        std::string step_direction = SGeographicUtils::BearingToDirection(step_bearing);
        
        // get street name
        std::string step_street_name = GetStreetName(curID, nextID, DImplementation->StreetMap.get());

        // determine whether make change
        bool is_not_first_step = (i > 0);
        bool mode_is_changed = (mode != cur_mode);
        bool street_is_changed = (step_street_name != cur_street);
        bool direction_is_changed = (step_direction != cur_dir);

        // if mode / street name / direction change, add one description
        if (is_not_first_step && (mode_is_changed || street_is_changed || direction_is_changed)) {
            std::string mode_string;
            if (cur_mode == CTransportationPlanner::ETransportationMode::Bike) {
                mode_string = "Bike ";
            } else {
                mode_string = "Walk ";
            }

            std::string connect_word;
            std::string target_name;

            if (cur_street != "") {
                // 'along' if have stree name
                connect_word = " along ";
                target_name = cur_street;
            } else {
                //'toward' if dont have street name
                connect_word = " toward ";
                if (step_street_name != "") {
                    target_name = step_street_name;
                } 
                else {
                    target_name = "End";
                }
            }

            // format distance number
            char distance_buffer[64];
            sprintf(distance_buffer, "%.1f", cur_dist);

            // format description
            std::string walk_bike_result = mode_string + cur_dir + connect_word + target_name + " for " + distance_buffer + " mi";
            desc.push_back(walk_bike_result);
            
            cur_dist = 0.0;
        }

        //update variable
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
        } 
        else {
            final_mode_name = "Walk ";
        }

        // along / toward
        std::string connect_word;
        std::string final_place_name;

        if (cur_street != "") {
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
        std::string final_result = final_mode_name + cur_dir + connect_word + final_place_name + " for " + final_dist_buffer + " mi";
        desc.push_back(final_result);
    }

    // dest
    // get last node id, object, return true when there is dest node obj
    TNodeID last_node_id = path.back().second;
    std::shared_ptr<CStreetMap::SNode> last_node_object = DImplementation->StreetMap->NodeByID(last_node_id);
    if (last_node_object != nullptr) {
        std::string final_location_str = SGeographicUtils::ConvertLLToDMS(last_node_object->Location());
        desc.push_back("End at " + final_location_str);
    }

    return true;
}