#include "CSVBusSystem.h"

#include "BusSystem.h"

#include "DSVReader.h"
#include "DSVWriter.h"
#include <algorithm>
#include <unordered_map>
#include <utility>

struct CCSVBusSystem::SImplementation {
    struct SStop : CBusSystem::SStop {
        TStopID DStopID;
        CStreetMap::TNodeID DNodeID;

        SStop(const TStopID id, const CStreetMap::TNodeID nodeId) noexcept
            : DStopID(id), DNodeID(nodeId) {}

        ~SStop() noexcept override = default;

        TStopID ID() const noexcept override { return DStopID; }
        CStreetMap::TNodeID NodeID() const noexcept override { return DNodeID; }

        friend bool operator==(const SStop &lhs, const SStop &rhs) {
            return lhs.DStopID == rhs.DStopID;
        }
    };

    struct SRoute : CBusSystem::SRoute {
        std::string DName;
        std::vector<TStopID> DStops;

        SRoute(std::string name, std::vector<TStopID> stops)
            : DName(std::move(name)), DStops(std::move(stops)) {}

        ~SRoute() noexcept override = default;

        std::string Name() const noexcept override { return DName; }
        std::size_t StopCount() const noexcept override {
            return DStops.size();
        }

        TStopID GetStopID(const std::size_t index) const noexcept override {
            if (index >= DStops.size())
                return InvalidStopID;
            return DStops[index];
        }

        friend bool operator==(const SRoute &lhs, const SRoute &rhs) {
            return lhs.DName == rhs.DName;
        }
    };

    std::unordered_map<std::string, std::shared_ptr<SRoute>> DRoutesByName;
    std::vector<std::shared_ptr<SRoute>> DRoutesByIndex;
    std::vector<SStop> stops;

    std::size_t RouteCount() const noexcept { return DRoutesByName.size(); }

    std::size_t StopCount() const noexcept { return stops.size(); }

    std::shared_ptr<SStop> StopByIndex(const std::size_t index) const noexcept {
        if (index >= stops.size())
            return nullptr;
        return std::make_shared<SStop>(stops[index]);
    }

    std::shared_ptr<SStop> StopByID(const TStopID id) const noexcept {
        const auto it =
            std::find_if(stops.cbegin(), stops.cend(),
                         [id](const auto &stop) { return stop.ID() == id; });
        if (it == stops.cend())
            return nullptr;
        return std::make_shared<SStop>(*it);
    }

    std::shared_ptr<SRoute>
    RouteByIndex(const std::size_t index) const noexcept {
        if (index >= DRoutesByIndex.size())
            return nullptr;
        return DRoutesByIndex[index];
    }

    std::shared_ptr<SRoute>
    RouteByName(const std::string &name) const noexcept {
        const auto it = DRoutesByName.find(name);
        return it == DRoutesByName.cend() ? nullptr : it->second;
    }
};

CCSVBusSystem::~CCSVBusSystem() = default;

std::size_t CCSVBusSystem::RouteCount() const noexcept {
    return DImplementation->RouteCount();
}

std::size_t CCSVBusSystem::StopCount() const noexcept {
    return DImplementation->StopCount();
}

std::shared_ptr<CBusSystem::SRoute>
CCSVBusSystem::RouteByIndex(const std::size_t index) const noexcept {
    return DImplementation->RouteByIndex(index);
}

std::shared_ptr<CBusSystem::SRoute>
CCSVBusSystem::RouteByName(const std::string &name) const noexcept {
    return DImplementation->RouteByName(name);
}

std::shared_ptr<CBusSystem::SStop>
CCSVBusSystem::StopByID(const TStopID id) const noexcept {
    return DImplementation->StopByID(id);
}

std::shared_ptr<CBusSystem::SStop>
CCSVBusSystem::StopByIndex(const std::size_t index) const noexcept {
    return DImplementation->StopByIndex(index);
}

CCSVBusSystem::CCSVBusSystem(const std::shared_ptr<CDSVReader> &stopSource,
                             const std::shared_ptr<CDSVReader> &routeSource) {
    DImplementation = std::make_unique<SImplementation>();

    std::vector<std::string> buf;
    if (stopSource->ReadRow(buf)) { // Skip header
        buf.clear();
        while (stopSource->ReadRow(buf)) {
            if (buf.size() >= 2) {
                // emplace_back avoids extra alloc
                DImplementation->stops.emplace_back(std::stoul(buf[0]),
                                                    std::stoul(buf[1]));
            }
            buf.clear();
        }
    }

    buf.clear();

    if (routeSource->ReadRow(buf)) { // Skip header
        buf.clear();
        bool hasData = routeSource->ReadRow(buf);
        while (hasData) {
            if (buf.size() >= 2) {
                std::string routeName = buf[0];
                std::vector<TStopID> routeStops;

                // Group all consecutive rows with the same route name
                while (hasData && buf.size() >= 2 && buf[0] == routeName) {
                    routeStops.emplace_back(std::stoul(buf[1]));
                    buf.clear();
                    hasData = routeSource->ReadRow(buf);
                }

                const auto route = std::make_shared<SImplementation::SRoute>(
                    routeName, std::move(routeStops));

                DImplementation->DRoutesByName[routeName] = route;
                DImplementation->DRoutesByIndex.emplace_back(route);
            } else {
                // Ensure we always progress even if a row is malformed
                buf.clear();
                hasData = routeSource->ReadRow(buf);
            }
        }
    }
}
