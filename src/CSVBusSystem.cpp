#include "CSVBusSystem.h"

#include "BusSystem.h"

#include "DSVReader.h"
#include "DSVWriter.h"
#include <algorithm>
#include <ranges>
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

        TStopID GetStopID(std::size_t index) const noexcept override {
            if (index >= DStops.size())
                return CBusSystem::InvalidStopID;
            return DStops[index];
        }

        friend bool operator==(const SRoute &lhs, const SRoute &rhs) {
            return lhs.DName == rhs.DName;
        }
    };

    std::vector<std::shared_ptr<SRoute>> routes;
    std::vector<SStop> stops;

    std::size_t RouteCount() const noexcept { return routes.size(); }

    std::size_t StopCount() const noexcept { return stops.size(); }

    std::shared_ptr<SStop> StopByIndex(const std::size_t index) const noexcept {
        if (index >= stops.size())
            return nullptr;
        return std::make_shared<SStop>(stops[index]);
    }

    std::shared_ptr<SStop> StopByID(const TStopID id) const noexcept {
        const auto it = std::ranges::find(stops, id, &SStop::DStopID);
        if (it == stops.end())
            return nullptr;
        return std::make_shared<SStop>(*it);
    }

    std::shared_ptr<SRoute>
    RouteByIndex(const std::size_t index) const noexcept {
        if (index >= routes.size())
            return nullptr;
        return routes[index];
    }

    std::shared_ptr<SRoute>
    RouteByName(const std::string &name) const noexcept {
        const auto it = std::ranges::find(routes, name, &SRoute::DName);
        if (it == routes.end())
            return nullptr;
        return *it;
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
        while (stopSource->ReadRow(buf)) {
            if (buf.size() >= 2) {
                // emplace_back avoids extra alloc
                DImplementation->stops.emplace_back(std::stoul(buf[0]),
                                                    std::stoul(buf[1]));
            }
        }
    }

    if (routeSource->ReadRow(buf)) { // Skip header
        bool hasData = routeSource->ReadRow(buf);
        while (hasData) {
            if (buf.size() >= 2) {
                std::string routeName = buf[0];
                std::vector<TStopID> routeStops;

                // Group all consecutive rows with the same route name
                while (hasData && buf.size() >= 2 && buf[0] == routeName) {
                    routeStops.push_back(std::stoul(buf[1]));
                    hasData = routeSource->ReadRow(buf);
                }

                DImplementation->routes.push_back(
                    std::make_shared<SImplementation::SRoute>(routeName, std::move(routeStops)));
            } else {
                // Ensure we always progress even if a row is malformed
                hasData = routeSource->ReadRow(buf);
            }
        }
    }
}
