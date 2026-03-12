# CDijkstraPathRouter

This class extends CPathRouter and implements the Dijkstra algorithm to find the shortest path between two points based on the weights of the edges.

## Public Interface
### CDijkstraPathRouter()

The constructor just initializes the internal implementation for the path router.

### std::size_t VertexCount() const noexcept

Returns the total number of vertices that have been added to the router.

### TVertexID AddVertex(std::any tag) noexcept

Adds a vertex to the graph and lets you store a tag (like a node ID) with it. It returns a unique ID for that vertex.

### std::any GetVertexTag(TVertexID id) const noexcept

Gets the tag info back for a specific vertex. If the ID is wrong, it just returns an empty object.

### bool AddEdge(TVertexID src, TVertexID dest, double weight, bool bidir = false) noexcept

Connects two vertices together.

weight: The "cost" to travel. It cannot be negative.

bidir: If this is true, the code adds the edge for both directions.

Returns false if the weight is bad or the IDs don't exist.

### bool Precompute(std::chrono::steady_clock::time_point deadline) noexcept

Gives the router some time to do math ahead of time. It has to stop when it hits the deadline.

### double FindShortestPath(TVertexID src, TVertexID dest, std::vector<TVertexID> &path) noexcept

This is the main part that runs the Dijkstra math.

It fills the path vector with the best sequence of IDs.

Returns the total weight. If there is no way to get there, it returns NoPathExists.

## Code Example

```cpp
CDijkstraPathRouter my_router;

TVertexID p1 = my_router.AddVertex(101ull);
TVertexID p2 = my_router.AddVertex(102ull);

my_router.AddEdge(p1, p2, 5.0, true);

std::vector<TVertexID> result_path;
double total_cost = my_router.FindShortestPath(p1, p2, result_path);
```