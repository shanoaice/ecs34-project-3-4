#ifndef OPENSTREETMAP_H
#define OPENSTREETMAP_H

#include "XMLReader.h"
#include "StreetMap.h"

class COpenStreetMap : public CStreetMap{
    private:
        struct SImplementation;
        std::unique_ptr<SImplementation> DImplementation;

    public:
        COpenStreetMap(const std::shared_ptr<CXMLReader>& src);
        ~COpenStreetMap();

        std::size_t NodeCount() const noexcept override;
        std::size_t WayCount() const noexcept override;
        std::shared_ptr<SNode> SortedNodeByIndex(std::size_t index) const noexcept;
        std::shared_ptr<SNode> NodeByIndex(std::size_t index) const noexcept override;
        std::shared_ptr<SNode> NodeByID(TNodeID id) const noexcept override;
        std::shared_ptr<SWay> WayByIndex(std::size_t index) const noexcept override;
        std::shared_ptr<SWay> WayByID(TWayID id) const noexcept override;
};

#endif
