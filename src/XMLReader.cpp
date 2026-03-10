#include "XMLReader.h"
#include "StringUtils.h"
#include <expat.h>
#include <queue>
#include <utility>

// <tag attr="val">CharData</tag>

struct CXMLReader::SImplementation {
    std::shared_ptr<CDataSource> DSource;
    XML_Parser DParser;
    std::queue<SXMLEntity> DQueue;
    std::vector<char> DBuffer;
    const size_t DBufferSize = 512;

    // Store accumulated CharData that hasn't been returned yet
    std::string DAccumulatedCharData;

    bool DDone = false;
    bool DParseError = false;

    static void StartElementHandler(void *userData, const XML_Char *name,
                                    const XML_Char **atts) {
        auto *This = static_cast<SImplementation *>(userData);

        FlushCData(*This);

        SXMLEntity NewEntity;

        NewEntity.DType = SXMLEntity::EType::StartElement;
        NewEntity.DNameData = name;
        for (int i = 0; atts[i]; i += 2) {
            NewEntity.DAttributes.emplace_back(atts[i], atts[i + 1]);
        }
        This->DQueue.push(NewEntity);
    }

    static void EndElementHandler(void *userData, const XML_Char *name) {
        auto *This = static_cast<SImplementation *>(userData);

        FlushCData(*This);

        SXMLEntity NewEntity;

        NewEntity.DType = SXMLEntity::EType::EndElement;
        NewEntity.DNameData = name;
        This->DQueue.push(NewEntity);
    }

    static void FlushCData(SImplementation &self) {
        // Flush any accumulated character data before the end element
        if (!self.DAccumulatedCharData.empty()) {
            if (!StringUtils::IsAllWhiteSpace(self.DAccumulatedCharData)) {
                SXMLEntity CharEntity;
                CharEntity.DType = SXMLEntity::EType::CharData;
                CharEntity.DNameData = self.DAccumulatedCharData;
                self.DQueue.push(CharEntity);
            }
            self.DAccumulatedCharData.clear();
        }
    }

    static void CharacterDataHandler(void *userData, const XML_Char *cdata,
                                     const int len) {
        auto *This = static_cast<SImplementation *>(userData);

        This->DAccumulatedCharData += std::string(cdata, len);
    }

    SImplementation(std::shared_ptr<CDataSource> src) {
        DSource = std::move(src);
        DParser = XML_ParserCreate(nullptr);

        XML_SetUserData(DParser, (void *)this);
        XML_SetStartElementHandler(DParser, StartElementHandler);
        XML_SetEndElementHandler(DParser, EndElementHandler);
        XML_SetCharacterDataHandler(DParser, CharacterDataHandler);
    }

    ~SImplementation() { XML_ParserFree(DParser); }

    bool End() const { return DDone && DQueue.empty(); }

    bool OneTimePump() {
        if (DDone || DParseError) {
            return false;
        }

        DSource->Read(DBuffer, DBufferSize);

        const bool IsFinal = DSource->End();

        const int Len = static_cast<int>(DBuffer.size());
        const int Status =
            XML_Parse(DParser, DBuffer.empty() ? "" : DBuffer.data(), Len,
                      IsFinal ? 1 : 0);

        if (Status == XML_STATUS_ERROR) {
            DParseError = true;
            return false;
        }

        if (IsFinal) {
            FlushCData(*this);
            DDone = true;
        }
        return true;
    }

    bool ReadEntity(SXMLEntity &entity, bool skipcdata) {
        while (true) {

            if (skipcdata) {
                while (!DQueue.empty() &&
                       DQueue.front().DType == SXMLEntity::EType::CharData) {
                    DQueue.pop();
                }
            }

            if (DParseError) {
                return false;
            }

            if (!DQueue.empty()) {
                entity = DQueue.front();
                DQueue.pop();
                return true;
            }

            if (End()) {
                return false;
            }

            if (!OneTimePump()) {
                return false;
            }
        }

        return false;
    }
};

CXMLReader::CXMLReader(std::shared_ptr<CDataSource> src) {
    DImplementation = std::make_unique<SImplementation>(src);
}

CXMLReader::~CXMLReader() = default;

bool CXMLReader::End() const { return DImplementation->End(); }

bool CXMLReader::ReadEntity(SXMLEntity &entity, bool skipcdata) const {
    return DImplementation->ReadEntity(entity, skipcdata);
}
