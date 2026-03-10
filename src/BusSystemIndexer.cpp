#include "BusSystemIndexer.h"
#include <vector>
#include <unordered_map>
#include <algorithm>

struct CBusSystemIndexer::SImplementation {
    std::shared_ptr<CBusSystem> DBusSystem;
    std::vector<std::shared_ptr<SStop>> DSortedStopsByIndex;
    std::unordered_map<TNodeID, std::shared_ptr<SStop>> DStopsByNodeID;
    std::vector<std::shared_ptr<SRoute>> DSortedRoutesByIndex;

    // combine two ID, form a key for unordered_map, hash function
    struct SNodeIDPairHash {
        std::size_t operator()(const std::pair<TNodeID, TNodeID> &pair) const {
            std::size_t First = pair.first;
            std::size_t Second = pair.second;
            return First ^ (Second << 1);
        }
    };

    std::unordered_map<std::pair<TNodeID, TNodeID>, std::unordered_set<std::shared_ptr<SRoute>>, SNodeIDPairHash> DRoutesByNodeIDs;

    SImplementation(std::shared_ptr<CBusSystem> bussystem) {
        DBusSystem = bussystem;
        if (!DBusSystem) return;

        // loop through all stops, store in unordered_map
        for (size_t i = 0; i < DBusSystem->StopCount(); i++) {
            auto Stop = DBusSystem->StopByIndex(i);
            DSortedStopsByIndex.push_back(Stop);
            DStopsByNodeID[Stop->NodeID()] = Stop;
        }

        // sort by ID
        std::sort(DSortedStopsByIndex.begin(), DSortedStopsByIndex.end(), [](std::shared_ptr<SStop> l, std::shared_ptr<SStop> r) -> bool {
            return l->ID() < r->ID();
        });

        // loop through all route, store in vector
        for (size_t i = 0; i < DBusSystem->RouteCount(); i++) {
            auto Route = DBusSystem->RouteByIndex(i);
            DSortedRoutesByIndex.push_back(Route);
        }

        // sort by route name
        std::sort(DSortedRoutesByIndex.begin(), DSortedRoutesByIndex.end(), [](std::shared_ptr<SRoute> l, std::shared_ptr<SRoute> r) -> bool {
            return l->Name() < r->Name();
        });

        // loop through every route every segment, store in unordered_map
        for (auto Route : DSortedRoutesByIndex) {
            for (size_t i = 1; i < Route->StopCount(); i++) {
                // get stop id
                auto PrevID = Route->GetStopID(i - 1);
                auto CurrID = Route->GetStopID(i);
                // use stop id to get node id
                auto FirstNodeID = DBusSystem->StopByID(PrevID)->NodeID();
                auto SecondNodeID = DBusSystem->StopByID(CurrID)->NodeID();

                auto Key = std::make_pair(FirstNodeID, SecondNodeID);
                
                // search = true is key already exist
                auto Search = DRoutesByNodeIDs.find(Key);
        
                if (Search == DRoutesByNodeIDs.end()) { 
                    // if not exist, initialize
                    DRoutesByNodeIDs[Key] = {Route};
                }
                else { 
                    // if exist, insert new route
                    Search->second.insert(Route);
                }
            }
        }

    }

    ~SImplementation() {}

    std::size_t StopCount() const noexcept {
        return DSortedStopsByIndex.size();
    }

    std::size_t RouteCount() const noexcept {
        return DSortedRoutesByIndex.size();
    }

    // check valid index, return stop
    std::shared_ptr<SStop> SortedStopByIndex(std::size_t i) const noexcept {
        if (i >= DSortedStopsByIndex.size()) return nullptr;
        return DSortedStopsByIndex[i];
    }

    // check valid index, return route
    std::shared_ptr<SRoute> SortedRouteByIndex(std::size_t i) const noexcept {
        if (i >= DSortedRoutesByIndex.size()) return nullptr;
        return DSortedRoutesByIndex[i];
    }

    // check valid id, return stop
    std::shared_ptr<SStop> StopByNodeID(TNodeID id) const noexcept {
        auto Search = DStopsByNodeID.find(id);
        if (Search != DStopsByNodeID.end()) {
            return Search->second;
        }
        return nullptr;
    }


    // if found the src, dest pair, store routes, return bool
    bool RoutesByNodeIDs(TNodeID src, TNodeID dest, std::unordered_set<std::shared_ptr<SRoute>> &routes) const noexcept {
        routes.clear();
        auto Search = DRoutesByNodeIDs.find(std::make_pair(src, dest));
        // if found the src, dest pair, store routes
        if (Search != DRoutesByNodeIDs.end()) {
            routes = Search->second;
            return true;
        }
        return false;
    }

    // check whether there is route between two node
    bool RouteBetweenNodeIDs(TNodeID src, TNodeID dest) const noexcept {
        std::pair<TNodeID, TNodeID> Key = std::make_pair(src, dest);
        auto Search = DRoutesByNodeIDs.find(Key);
        if (Search != DRoutesByNodeIDs.end()) {
            return true;
        }
        else {
            return false;
        }
    }
};


CBusSystemIndexer::CBusSystemIndexer(std::shared_ptr<CBusSystem> bussystem){
    DImplementation = std::make_unique<SImplementation>(bussystem);
}

CBusSystemIndexer::~CBusSystemIndexer(){

}

std::size_t CBusSystemIndexer::StopCount() const noexcept{
    return DImplementation->StopCount();
}

std::size_t CBusSystemIndexer::RouteCount() const noexcept{
    return DImplementation->RouteCount();
}

std::shared_ptr<CBusSystemIndexer::SStop> CBusSystemIndexer::SortedStopByIndex(std::size_t i) const noexcept{
    return DImplementation->SortedStopByIndex(i);
}

std::shared_ptr<CBusSystemIndexer::SRoute> CBusSystemIndexer::SortedRouteByIndex(std::size_t i) const noexcept{
    return DImplementation->SortedRouteByIndex(i);
}

std::shared_ptr<CBusSystemIndexer::SStop> CBusSystemIndexer::StopByNodeID(TNodeID id) const noexcept{
    return DImplementation->StopByNodeID(id);
}

bool CBusSystemIndexer::RoutesByNodeIDs(TNodeID src, TNodeID dest, std::unordered_set<std::shared_ptr<SRoute> > &routes) const noexcept{
    return DImplementation->RoutesByNodeIDs(src,dest,routes);
}

bool CBusSystemIndexer::RouteBetweenNodeIDs(TNodeID src, TNodeID dest) const noexcept{
    return DImplementation->RouteBetweenNodeIDs(src,dest);
}
