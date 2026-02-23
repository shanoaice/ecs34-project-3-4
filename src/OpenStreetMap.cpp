#include "OpenStreetMap.h"

#include <algorithm>
#include <unordered_map>
#include <utility>
// define a constant to handle invalid node id
const CStreetMap::TNodeID CStreetMap::InvalidNodeID;

struct COpenStreetMap::SImplementation {

    const std::string DOSMTag = "osm";
    const std::string DNodeTag = "node";
    const std::string DNodeIDAttr = "id";
    const std::string DNodeLatAttr = "lat";
    const std::string DNodeLonAttr = "lon";
    const std::string DTagTag = "tag";
    const std::string DWayTag = "way";
    const std::string DWayIDAttr = "id";
    const std::string DWayNodeTag = "nd";
    const std::string DWayNodeAtt = "ref";

    struct SNode : public CStreetMap::SNode {
        TNodeID DID;
        TLocation DLocation;
        std::vector<std::pair<std::string, std::string>> DAttributes;
        ~SNode() override = default;
        TNodeID ID() const noexcept override { return DID; }

        TLocation Location() const noexcept override { return DLocation; }

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
            return std::ranges::any_of(DAttributes,
                                       [key](const auto &attr_pair) {
                                           return attr_pair.first == key;
                                       });
        }

        std::string
        GetAttribute(const std::string &key) const noexcept override {
            const auto it = std::ranges::find(
                DAttributes, key, &std::pair<std::string, std::string>::first);
            return it == DAttributes.end() ? "" : it->second;
        }
    };

    struct SWay : public CStreetMap::SWay {
        ~SWay() override = default;
        TWayID DID;
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
            return std::ranges::any_of(DAttributes,
                                       [key](const auto &attr_pair) {
                                           return attr_pair.first == key;
                                       });
        }

        std::string
        GetAttribute(const std::string &key) const noexcept override {
            const auto it = std::ranges::find(DAttributes, key, &std::pair<std::string, std::string>::first);
            return it == DAttributes.end() ? "" : it->second;
        }
    };

    std::vector<std::shared_ptr<SNode>> DNodesByIndex;
    std::unordered_map<TNodeID, std::shared_ptr<SNode>> DNodesByID;

    bool ParseNodes(const std::shared_ptr<CXMLReader> &src, SXMLEntity &nextentity) {
        SXMLEntity TempEntity;
        // loop until find <node> start element
        while (src->ReadEntity(TempEntity)) {
            if (TempEntity.DType == SXMLEntity::EType::StartElement) {
                // if it is start, but not <node>, pass to nextentity
                if (TempEntity.DNameData != DNodeTag) {
                    nextentity = TempEntity;
                    return true;
                }

                // is start node
                else {
                    // store id, lat, lon
                    auto id_str = TempEntity.AttributeValue(DNodeIDAttr);
                    auto lat_str = TempEntity.AttributeValue(DNodeLatAttr);
                    auto lon_str = TempEntity.AttributeValue(DNodeLonAttr);
                    if (!id_str.empty() && !lat_str.empty() &&
                        !lon_str.empty()) {
                        auto NewNode = std::make_shared<SNode>();
                        NewNode->DID = std::stoull(id_str);
                        NewNode->DLocation = {std::stod(lat_str),
                                              std::stod(lon_str)};

                        // parse subentity
                        SXMLEntity SubEntity;
                        while (src->ReadEntity(SubEntity)) {
                            // when reach end node, break
                            if (SubEntity.DType ==
                                    SXMLEntity::EType::EndElement &&
                                SubEntity.DNameData == DNodeTag) {
                                break;
                            }
                            // when reach start tag, store in DAttributes
                            if (SubEntity.DType ==
                                    SXMLEntity::EType::StartElement &&
                                SubEntity.DNameData == DTagTag) {
                                std::string key = SubEntity.AttributeValue("k");
                                std::string val = SubEntity.AttributeValue("v");
                                NewNode->DAttributes.emplace_back(key, val);
                            }
                        }
                        // store Node into DNodesByIndex and DNodesByID
                        DNodesByIndex.push_back(NewNode);
                        DNodesByID[NewNode->DID] = NewNode;
                    }
                }
            }
        }
        return true;
    }

    std::vector<std::shared_ptr<SWay>> DWaysByIndex;
    std::unordered_map<TWayID, std::shared_ptr<SWay>> DWaysByID;

    bool ParseWays(const std::shared_ptr<CXMLReader>& src, SXMLEntity &firstentity) {
        // first way entity
        if (firstentity.DType == SXMLEntity::EType::StartElement &&
            firstentity.DNameData == DWayTag) {
            // extract Way ID
            auto id_str = firstentity.AttributeValue(DWayIDAttr);
            if (!id_str.empty()) {
                auto NewWay = std::make_shared<SWay>();
                auto WayID = std::stoull(id_str);
                NewWay->DID = WayID;

                // sub enetity of first Way entity
                SXMLEntity SubEntity;
                while (src->ReadEntity(SubEntity)) {
                    // when reach endelement </way>, end
                    if (SubEntity.DType == SXMLEntity::EType::EndElement &&
                        SubEntity.DNameData == DWayTag) {
                        break;
                    }
                    // when reach <nd ...>, store in DNodeID
                    if (SubEntity.DType == SXMLEntity::EType::StartElement &&
                        SubEntity.DNameData == DWayNodeTag) {
                        std::string ref = SubEntity.AttributeValue(DWayNodeAtt);
                        if (!ref.empty()) {
                            auto ref_num = std::stoull(ref);
                            NewWay->DNodeID.push_back(ref_num);
                        }
                    }
                    // when reach <tag ...>, store in DAttributes
                    if ((SubEntity.DType == SXMLEntity::EType::StartElement &&
                         SubEntity.DNameData == DTagTag)) {
                        std::string key = SubEntity.AttributeValue("k");
                        std::string val = SubEntity.AttributeValue("v");
                        NewWay->DAttributes.emplace_back(key, val);
                    }
                }
                // store first way entity into DWaysByIndex and DWaysByID
                DWaysByIndex.push_back(NewWay);
                DWaysByID[NewWay->DID] = NewWay;
            }
        }

        // Remaining Way entity
        SXMLEntity TempEntity;
        while (src->ReadEntity(TempEntity)) {
            // if reach </osm>, end
            if (TempEntity.DType == SXMLEntity::EType::EndElement &&
                TempEntity.DNameData == DOSMTag) {
                break;
            }

            // find new <way> entity
            if (TempEntity.DType == SXMLEntity::EType::StartElement &&
                TempEntity.DNameData == DWayTag) {
                auto id_str = TempEntity.AttributeValue(DWayIDAttr);
                // check not empty
                if (!id_str.empty()) {
                    // store way ID
                    auto NewWay = std::make_shared<SWay>();
                    NewWay->DID = std::stoull(id_str);

                    // parese sub entity
                    SXMLEntity SubEntity;
                    while (src->ReadEntity(SubEntity)) {
                        // if reach way end element, break
                        if (SubEntity.DType == SXMLEntity::EType::EndElement &&
                            SubEntity.DNameData == DWayTag) {
                            break;
                        }
                        // if start element
                        if (SubEntity.DType ==
                            SXMLEntity::EType::StartElement) {
                            // if the start element is node info, store
                            // reference number
                            if (SubEntity.DNameData == DWayNodeTag) {
                                std::string ref =
                                    SubEntity.AttributeValue(DWayNodeAtt);
                                if (!ref.empty()) {
                                    NewWay->DNodeID.push_back(std::stoull(ref));
                                }
                            }
                            // if the start element is tag, store in DAttributes
                            else if (SubEntity.DNameData == DTagTag) {
                                std::string key = SubEntity.AttributeValue("k");
                                std::string val = SubEntity.AttributeValue("v");
                                NewWay->DAttributes.emplace_back(key, val);
                            }
                        }
                    }
                    DWaysByIndex.push_back(NewWay);
                    DWaysByID[NewWay->DID] = NewWay;
                }
            }
        }
        return true;
    }

    bool ParseOpenStreetMap(const std::shared_ptr<CXMLReader> &src) {
        SXMLEntity TempEntity;
        // loop until the starte element <osm> found
        while (src->ReadEntity(TempEntity)) {
            if (TempEntity.DType == SXMLEntity::EType::StartElement &&
                TempEntity.DNameData == DOSMTag) {
                SXMLEntity NextEntity;
                // parse <node>, until reach <way>, put <way> into NextEntity
                if (ParseNodes(src, NextEntity)) {
                    // parse <way>
                    if (ParseWays(src, NextEntity)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    explicit SImplementation(const std::shared_ptr<CXMLReader>& src) {
        if (ParseOpenStreetMap(src)) {
        }
    }

    // total node parsed
    std::size_t NodeCount() const noexcept {
        return DNodesByIndex.size();
        ;
    }

    // total way parsed
    std::size_t WayCount() const noexcept { return DWaysByIndex.size(); }

    std::shared_ptr<CStreetMap::SNode>
    NodeByIndex(const std::size_t index) const noexcept {
        // bound check
        if (index < DNodesByIndex.size()) {
            return DNodesByIndex[index];
        }
        // if out of range, return null pointer
        return nullptr;
    }

    // find node by node id
    std::shared_ptr<CStreetMap::SNode> NodeByID(TNodeID id) const noexcept {
        auto it = DNodesByID.find(id);
        if (it != DNodesByID.end()) {
            return it->second;
        }
        return nullptr;
    }

    std::shared_ptr<CStreetMap::SWay>
    WayByIndex(const std::size_t index) const noexcept {
        // bound check
        if (index < DWaysByIndex.size()) {
            return DWaysByIndex[index];
        }
        return nullptr;
    }

    std::shared_ptr<CStreetMap::SWay> WayByID(TWayID id) const noexcept {
        // find way by way id
        auto it = DWaysByID.find(id);
        if (it != DWaysByID.end()) {
            return it->second;
        }
        return nullptr;
    }
};

COpenStreetMap::COpenStreetMap(const std::shared_ptr<CXMLReader>& src) {
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
