# CSVBusSystem

This class parses a CSV file with the following format (and header, mandatory!) `stop_id, node_id` for stops, and `route, stop_id` for routes.

## `std::size_t StopCount()`

Returns the total number of stops.

## `std::size_t RouteCount()`

Returns the total number of routes.

## `std::shared_ptr<SStop> StopByIndex(std::size_t index)`

Returns the SStop object at given index. Index out of bounds will return `nullptr`.

## `std::shared_ptr<SStop> StopByID(TStopID id)`

Returns the SStop object with given id. Nonexistent id will return `nullptr`.

## `std::shared_ptr<SRoute> RouteByIndex(std::size_t index)`

Returns the SRoute object at given index. Index out of bounds will return `nullptr`.

## `std::shared_ptr<SRoute> RouteByName(const std::string &name)`

Returns the SRoute object with given name. Nonexistent name will return `nullptr`.
