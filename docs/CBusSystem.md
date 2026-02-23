# CBusSystem

This class represents a bus system with a set of bus stops grouped in a set of bus routes.

## Public Interface

### `std::size_t CBusSystem::StopCount()`

Returns the total number of stops.

### `std::size_t CBusSystem::RouteCount()`

Returns the total number of routes.

### `std::shared_ptr<SStop> CBusSystem::StopByIndex(std::size_t index)`

Returns the SStop object at given index. Index out of bounds will return `nullptr`.

### `std::shared_ptr<SStop> CBusSystem::StopByID(TStopID id)`

Returns the SStop object with given id. Nonexistent id will return `nullptr`.

### `std::shared_ptr<SRoute> CBusSystem::RouteByIndex(std::size_t index)`

Returns the SRoute object at given index. Index out of bounds will return `nullptr`.

### `std::shared_ptr<SRoute> CBusSystem::RouteByName(const std::string &name)`

Returns the SRoute object with given name. Nonexistent name will return `nullptr`.

### `TStopID CBusSystem::SStop::ID()`

Returns the ID of the bus stop.

### `COpenStreetMap::TNodeID CBusSystem::SStop::NodeID()`

Returns the map node ID of the bus stop.

### `std::string CBusSystem::SRoute::Name()`

Returns the route name.

### `std::size_t CBusSystem::SRoute::StopCount()`

Returns the number of stops this route contains.

### `TStopID CBusSystem::SRoute::GetStopID(std::size_t index)`

Gets the id of the stop at given index
