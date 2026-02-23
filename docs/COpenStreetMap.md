# COpenStreetMap

This class extends the [`CStreetMap`](./CStreetMap.md) class and constructs the map structure by parsing an OSM format XML file.

## Public Interface

### `COpenStreetMap::COpenStreetMap(std::shared_ptr<CXMLReader> src)`

This function initializes the OpenStreetMap object with the given XMLReader source.

No other extra public interfaces provided. Refer to base class doc.

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
