# CPathRouter
This is a base class for path finding. It is an abstract class, so you cannot make an object of it directly. It defines the basic functions that any path router (like Dijkstra) should have.

## Public Interface
### virtual ~CPathRouter()

The destructor for the class. It is virtual so that child classes can clean up their memory correctly.

### using TVertexID = std::size_t

This is just a nickname for std::size_t, used for vertex IDs in the graph.

### static constexpr TVertexID InvalidVertexID

A constant that means the ID is not valid. It uses the max value of size_t.

### static constexpr double NoPathExists

A constant that the router return when it cannot find any path between two points. It is the max value of double.

### virtual std::size_t VertexCount() const noexcept = 0

Return how many vertices (points) we have added to the graph.

### virtual TVertexID AddVertex(std::any tag) noexcept = 0

Add a new vertex to the graph.

tag: You can store anything here (like a node ID).

It return the ID of the new vertex.

### virtual std::any GetVertexTag(TVertexID id) const noexcept = 0

You give it a vertex ID, and it give you back the tag you stored.

### virtual bool AddEdge(TVertexID src, TVertexID dest, double weight, bool bidir = false) noexcept = 0

This is for linking two points with an edge.

src & dest: Start and end points of the edge.

weight: The cost to go through this edge.

bidir: If it is true, it add edges for both directions.

It return false if IDs are wrong or weight is negative.

### virtual bool Precompute(std::chrono::steady_clock::time_point deadline) noexcept = 0

This lets the router do some math work before we actually call the search function. It must stop when it reach the deadline.

### virtual double FindShortestPath(TVertexID src, TVertexID dest, std::vector<TVertexID> &path) noexcept = 0

The most important function to find the shortest path.

path: It will be filled with the vertex IDs of the path.

Returns the total weight. If no path is found, it return NoPathExists.