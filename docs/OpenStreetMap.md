# CXMLWriter

This class can parse OpenStreetMap(OSM) from XML source.
Can store Nodes and Ways in vector for lookup.

## Functions

### `COpenStreetMap::COpenStreetMap(std::shared_ptr<CXMLReader> src)`

This function itializes the OpenStreetMap object.

### `std::size_t COpenStreetMap::NodeCount() const noexcept`

Return the number of nodes been parsed

### `std::size_t COpenStreetMap::WayCount() const noexcept`

Return the number of ways been parsed

### `std::shared_ptr<CStreetMap::SNode> COpenStreetMap::NodeByIndex(std::size_t index) const noexcept`

Takes in an index used to find the corresponding node based on parse order
Return nullptr if index out of range.

### `std::shared_ptr<CStreetMap::SNode> COpenStreetMap::NodeByID(TNodeID id) const noexcept`

Takes in a node id used to find the node through unordered_map

### `std::shared_ptr<CStreetMap::SWay> COpenStreetMap::WayByIndex(std::size_t index) const noexcept`

Takes in an index, used to find the corresponding way based on parse order
Return nullptr if index out of range.

### `std::shared_ptr<CStreetMap::SWay> COpenStreetMap::WayByID(TWayID id) const noexcept`

Takes in a way ID, used to find corresponding way

## Code Example

```cpp
COpenStreetMap MyMap(Reader);

auto Node = MyMap.NodeByID(62208369ull);
if (Node) {
    auto location = Node->Location()
    double lat = std::get<0>(location);
    double lon = std::get<1>(location);
    std::string Attr = Node->GetAttribute("highway");
}

size_t NumWays = MyMap.WayCount();
```
