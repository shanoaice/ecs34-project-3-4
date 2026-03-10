#include "OpenStreetMap.h"
#include "XMLEntity.h"
#include "XMLReader.h"
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

// define a constant to handle invalid node id
const CStreetMap::TNodeID CStreetMap::InvalidNodeID;

struct COpenStreetMap::SImplementation {

    struct SNode : public CStreetMap::SNode {
        TNodeID DID = InvalidNodeID;
        SLocation DLocation;
        std::vector<std::pair<std::string, std::string>> DAttributes;
        ~SNode() override = default;
        TNodeID ID() const noexcept override { return DID; }

        SLocation Location() const noexcept override { return DLocation; }

        std::size_t AttributeCount() const noexcept override {
            return DAttributes.size();
        }

        std::string GetAttributeKey(std::size_t index) const noexcept override {
            // bound check
            if (index < DAttributes.size()) {
                return DAttributes[index].first;
            } else {
                return "";
            }
        }

        bool HasAttribute(const std::string &key) const noexcept override {
            // std::ranges::any_of is better than simply loop and match
            return std::any_of(DAttributes.cbegin(), DAttributes.cend(),
                               [key](const auto &attr_pair) {
                                   return attr_pair.first == key;
                               });
        }

        std::string
        GetAttribute(const std::string &key) const noexcept override {
            const auto it =
                std::find_if(DAttributes.cbegin(), DAttributes.cend(),
                             [key](const auto &attr_pair) {
                                 return attr_pair.first == key;
                             });
            return it == DAttributes.cend() ? "" : it->second;
        }
    };

    struct SWay : public CStreetMap::SWay {
        ~SWay() override = default;
        TWayID DID = InvalidWayID;
        std::vector<TNodeID> DNodeID;
        std::vector<std::pair<std::string, std::string>> DAttributes;

        TWayID ID() const noexcept override { return DID; }

        std::size_t NodeCount() const noexcept override {
            return DNodeID.size();
        }

        TNodeID GetNodeID(std::size_t index) const noexcept override {
            // check boundary
            if (index < DNodeID.size()) {
                return DNodeID[index];
            } else {
                return InvalidNodeID;
            }
        }

        std::size_t AttributeCount() const noexcept override {
            return DAttributes.size();
        }

        std::string GetAttributeKey(std::size_t index) const noexcept override {
            // bound check
            if (index < DAttributes.size()) {
                // first in the attribute pair is key
                return DAttributes[index].first;
            } else {
                return "";
            }
        }

        bool HasAttribute(const std::string &key) const noexcept override {
            return std::any_of(DAttributes.cbegin(), DAttributes.cend(),
                               [key](const auto &attr_pair) {
                                   return attr_pair.first == key;
                               });
        }

        std::string
        GetAttribute(const std::string &key) const noexcept override {
            const auto it =
                std::find_if(DAttributes.cbegin(), DAttributes.cend(),
                             [key](const auto &attr_pair) {
                                 return attr_pair.first == key;
                             });
            return it == DAttributes.cend() ? "" : it->second;
        }
    };

    std::vector<std::shared_ptr<SWay>> DWays;
    std::unordered_map<TWayID, std::shared_ptr<SWay>> DWaysById;

    std::vector<std::shared_ptr<SNode>> DNodes;
    std::unordered_map<TNodeID, std::shared_ptr<SNode>> DNodesById;

    bool ParseOpenStreetMap(const std::shared_ptr<CXMLReader> &src) {
        // parse until OSM tag is found
        SXMLEntity currentEntity;
        src->ReadEntity(currentEntity);
        while (currentEntity.DType != SXMLEntity::EType::StartElement ||
               currentEntity.DNameData != "osm") {
            bool success = src->ReadEntity(currentEntity);
            if (!success)
                return false;
        }

        // now read the OSM file
        while (src->ReadEntity(currentEntity)) {
            if (currentEntity.DType == SXMLEntity::EType::StartElement &&
                currentEntity.DNameData == "node") {
                // call a function to parse the current node block
                ParseNode(src, currentEntity);
            } else if (currentEntity.DType == SXMLEntity::EType::StartElement &&
                       currentEntity.DNameData == "way") {
                // call a function to parse the current way block
                ParseWay(src, currentEntity);
            }
        }

        return true;
    }

    void ParseNode(const std::shared_ptr<CXMLReader> &src,
                           SXMLEntity &currentEntity) {
        SNode node;
        node.DID = std::stoull(currentEntity.AttributeValue("id"));
        node.DLocation = {
            std::stod(currentEntity.AttributeValue("lat")),
            std::stod(currentEntity.AttributeValue("lon"))}; // done
        while (src->ReadEntity(currentEntity)) {
            if (currentEntity.DType == SXMLEntity::EType::EndElement &&
                currentEntity.DNameData == "node") {
                break;
            } else if (currentEntity.DType == SXMLEntity::EType::StartElement &&
                       currentEntity.DNameData == "tag") {
                // parse tag
                std::string key = currentEntity.AttributeValue("k");
                std::string val = currentEntity.AttributeValue("v");
                // emplace_back avoids allocation
                node.DAttributes.emplace_back(key, val);
                // then, consume and discard anything until we see the matching
                // closing tag
                while (currentEntity.DType != SXMLEntity::EType::EndElement ||
                       currentEntity.DNameData != "tag") {
                    if (!src->ReadEntity(currentEntity))
                        break;
                }
            }
        }

        const auto nodePtr = std::make_shared<SNode>(node);

        DNodes.emplace_back(nodePtr);
        DNodesById[node.DID] = nodePtr;
    }

    void ParseWay(const std::shared_ptr<CXMLReader> &src,
                          SXMLEntity &currentEntity) {
        SWay way;
        way.DID = std::stoull(currentEntity.AttributeValue("id"));
        while (src->ReadEntity(currentEntity)) {
            if (currentEntity.DType == SXMLEntity::EType::EndElement &&
                currentEntity.DNameData == "way") {
                break;
            } else if (currentEntity.DType == SXMLEntity::EType::StartElement &&
                       currentEntity.DNameData == "nd") {
                // parse nodes
                std::string ref = currentEntity.AttributeValue("ref");
                if (!ref.empty()) {
                    way.DNodeID.push_back(std::stoull(ref));
                }
                // then, consume and discard anything until we see the matching
                // closing nd
                while (currentEntity.DType != SXMLEntity::EType::EndElement ||
                       currentEntity.DNameData != "nd") {
                    if (!src->ReadEntity(currentEntity))
                        break;
                }
            } else if (currentEntity.DType == SXMLEntity::EType::StartElement &&
                       currentEntity.DNameData == "tag") {
                // parse attributes
                std::string key = currentEntity.AttributeValue("k");
                std::string val = currentEntity.AttributeValue("v");
                way.DAttributes.emplace_back(key, val);
                // then, consume and discard anything until we see the matching
                // closing tag
                while (currentEntity.DType != SXMLEntity::EType::EndElement ||
                       currentEntity.DNameData != "tag") {
                    if (!src->ReadEntity(currentEntity))
                        break;
                }
            }
        }

        const auto wayPtr = std::make_shared<SWay>(way);

        DWays.push_back(wayPtr);
        DWaysById[way.DID] = wayPtr;
    }

    explicit SImplementation(const std::shared_ptr<CXMLReader> &src) {
        if (ParseOpenStreetMap(src)) {
        }
    }

    // total node parsed
    std::size_t NodeCount() const noexcept { return DNodes.size(); }

    // total way parsed
    std::size_t WayCount() const noexcept { return DWays.size(); }

    std::shared_ptr<CStreetMap::SNode>
    NodeByIndex(const std::size_t index) const noexcept {
        // bound check
        if (index < DNodes.size()) {
            return DNodes[index];
        }
        // if out of range, return null pointer
        return nullptr;
    }

    std::shared_ptr<CStreetMap::SNode>
    NodeByID(const TNodeID id) const noexcept {
        const auto it = DNodesById.find(id);

        return it == DNodesById.cend() ? nullptr : it->second;
    }

    std::shared_ptr<CStreetMap::SWay>
    WayByIndex(const std::size_t index) const noexcept {
        // bound check
        if (index < DWays.size()) {
            return DWays[index];
        }
        return nullptr;
    }

    std::shared_ptr<CStreetMap::SWay> WayByID(const TWayID id) const noexcept {
        // find way by way id
        const auto it = DWaysById.find(id);

        return it == DWaysById.cend() ? nullptr : it->second;
    }
};

COpenStreetMap::COpenStreetMap(const std::shared_ptr<CXMLReader> &src) {
    DImplementation = std::make_unique<SImplementation>(src);
}

COpenStreetMap::~COpenStreetMap() = default;

std::size_t COpenStreetMap::NodeCount() const noexcept {
    return DImplementation->NodeCount();
}
std::size_t COpenStreetMap::WayCount() const noexcept {
    return DImplementation->WayCount();
}

std::shared_ptr<CStreetMap::SNode>
COpenStreetMap::NodeByIndex(const std::size_t index) const noexcept {
    return DImplementation->NodeByIndex(index);
}

std::shared_ptr<CStreetMap::SNode>
COpenStreetMap::NodeByID(const TNodeID id) const noexcept {
    return DImplementation->NodeByID(id);
}
std::shared_ptr<CStreetMap::SWay>
COpenStreetMap::WayByIndex(const std::size_t index) const noexcept {
    return DImplementation->WayByIndex(index);
}

std::shared_ptr<CStreetMap::SWay>
COpenStreetMap::WayByID(const TWayID id) const noexcept {
    return DImplementation->WayByID(id);
}
