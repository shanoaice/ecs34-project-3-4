# CStreetMap

This class stores a stree map by grouping a set of nodes in a set of ways with certain attributes for identification and notation purposes.

## Public Interface

### `std::size_t CStreetMap::NodeCount() const noexcept`

Return the number of nodes been parsed

### `std::size_t CStreetMap::WayCount() const noexcept`

Return the number of ways been parsed

### `std::shared_ptr<CStreetMap::SNode> CStreetMap::NodeByIndex(std::size_t index) const noexcept`

Takes in an index used to find the corresponding node based on parse order
Return nullptr if index out of range.

### `std::shared_ptr<CStreetMap::SNode> CStreetMap::NodeByID(TNodeID id) const noexcept`

Takes in a node id used to find the node through unordered_map

### `std::shared_ptr<CStreetMap::SWay> CStreetMap::WayByIndex(std::size_t index) const noexcept`

Takes in an index, used to find the corresponding way based on parse order
Return nullptr if index out of range.

### `std::shared_ptr<CStreetMap::SWay> CStreetMap::WayByID(TWayID id) const noexcept`

Takes in a way ID, used to find corresponding way

### `TNodeID CStreetMap::SNode::ID()`

Returns the id of the map node.

### `TLocation CStreetMap::SNode::Location()`

Returns the location of the map node, in `std::pair<double, double>{lat, lon}` format.

### `std::size_t CStreetMap::SNode::AttributeCount()`

Returns the number of attributes in the map node.

### `std::string CStreetMap::SNode::GetAttributeKey(std::size_t index)`

Returns the key of the attribute at given index.

### `bool CStreetMap::SNode::HasAttribute(std::string k)`

Checks whether the given attribute key exists

### `std::string CStreetMap::SNode::GetAttribute(std::string k)`

Returns the value of the attribute at given key

### `TRouteID CStreetMap::SRoute::ID()`

Returns the id of the map route.

### `std::size_t CStreetMap::SRoute::NodeCount()`

Returns the number of nodes in this route.

### `TNodeID CStreetMap::SRoute::GetNodeID(std::size_t index)`

Returns the id of the node at given index.

### `std::size_t CStreetMap::SRoute::AttributeCount()`

Returns the number of attributes in the map Route.

### `std::string CStreetMap::SRoute::GetAttributeKey(std::size_t index)`

Returns the key of the attribute at given index.

### `bool CStreetMap::SRoute::HasAttribute(std::string k)`

Checks whether the given attribute key exists

### `std::string CStreetMap::SRoute::GetAttribute(std::string k)`

Returns the value of the attribute at given key
