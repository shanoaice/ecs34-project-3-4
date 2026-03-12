# CDijkstraTransportationPlanner
This class is the main part of project. It extends CTransportationPlanner. It combine StreetMap, BusSystem, and Dijkstra routers together to find paths. Also it turn trip data into human language.

## Public Interface
### CDijkstraTransportationPlanner(const std::shared_ptr<SConfiguration>& config)

Constructor. Very heavy here because need to setup three different routers:

shortest_router: only for distance.

walk_bus_router: for fastest time with walk and bus.

walk_bike_router: for fastest time with walk and bike.
Also init busSystemIndexer and edgeInfoMap for fast lookup.

### ~CDijkstraTransportationPlanner() override

Destructor. Just clear implementation.

### std::size_t NodeCount() const noexcept override

Return total nodes in map. Just ask StreetMap in config.

### std::shared_ptr<CStreetMap::SNode> SortedNodeByIndex(std::size_t index) const noexcept override

Get node by index, but nodes sorted by ID. If index too big, return nullptr.

### double FindShortestPath(TNodeID src, TNodeID dest, std::vector<TNodeID> &path) override

Find shortest distance (miles).

Use shortest_router.

If src same as dest, return 0.0, path only has one node.

### double FindFastestPath(TNodeID src, TNodeID dest, std::vector<TTripStep> &path) override

Find fastest time (hours).

Compare walk_bike_router and walk_bus_router time.

Pick the smaller one (faster).

Fill path with TTripStep (mode like walk, bike, bus and node ID).

### bool GetPathDescription(const std::vector<TTripStep> &path, std::vector<std::string> &desc) const override

Turn trip steps to text.

Calculate distance and direction (North, South...) for each step.

Combine steps on same street to make list shorter.

Use "along" if have street name, "toward" if no name.

Special for bus: tell which bus route and which stops.

## Code example
```cpp
CDijkstraTransportationPlanner planner(config);

std::vector<TNodeID> short_path;
double d = planner.FindShortestPath(1, 2, short_path);

std::vector<CTransportationPlanner::TTripStep> fast_path;
double t = planner.FindFastestPath(1, 2, fast_path);

std::vector<std::string> text;
if (planner.GetPathDescription(fast_path, text)) {
    // print "Start at..."
}
```
