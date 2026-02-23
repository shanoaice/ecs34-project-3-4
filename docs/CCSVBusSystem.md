# CCSVBusSystem

This class extends [`CBusSystem`](./CBusSystem.md), which constructs the bus system structure by parses a CSV file with the following format (and header, mandatory!) `stop_id, node_id` for stops, and `route, stop_id` for routes.

## Public Interface

### `CCSVBusSystem(std::shared_ptr<CDSVReader> stopSource, std::shared_ptr<CDSVReader> routeSource)`

Constructs the bus system given the two DSV(CSV) reader sources.

No other extra public interfaces implemented. Refer to base class document.
