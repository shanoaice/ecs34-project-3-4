# CTransportationPlanner
This class is the base interface for the transportation system. It defines what a planner must do, like finding shortest distance or fastest time. It is an abstract class, so you can't create it directly.

## Public Interface
### virtual ~CTransportationPlanner()

The destructor. It is virtual so when we delete the child class (like Dijkstra version), it clean up memory correctly.

### using TNodeID = CStreetMap::TNodeID

Just a easy name for Node ID from the street map.

### enum class ETransportationMode { Walk, Bike, Bus }

This is the three ways you can travel in this project.

### using TTripStep = std::pair<ETransportationMode, TNodeID>

A pair that store how you travel and where you are. It is used for the path in FindFastestPath.

### struct SConfiguration

This is a small class for setting up the planner. It give us:

StreetMap() and BusSystem() data.

Speed for WalkSpeed() and BikeSpeed().

BusStopTime() and other limit like DefaultSpeedLimit().

### virtual std::size_t NodeCount() const noexcept = 0

Return total number of nodes in map.

### virtual std::shared_ptr<CStreetMap::SNode> SortedNodeByIndex(std::size_t index) const noexcept = 0

Get node by index. Nodes are sorted by their ID. If index is wrong, return nullptr.

### virtual double FindShortestPath(TNodeID src, TNodeID dest, std::vector<TNodeID> &path) = 0

Find path with shortest miles.

src and dest are node IDs.

path will be filled with the nodes you pass.

Return: total miles. If no path, it return NoPathExists.

### virtual double FindFastestPath(TNodeID src, TNodeID dest, std::vector<TTripStep> &path) = 0

Find path with shortest time.

Use TTripStep so we know if we are walking, biking or taking bus.

Return: total time in hours.

### virtual bool GetPathDescription(const std::vector<TTripStep> &path, std::vector<std::string> &desc) const = 0

This one turn the path into human language like "Start at...", "Walk along...".

desc is the vector of strings it will fill.

Return: true if it successfully make the description.