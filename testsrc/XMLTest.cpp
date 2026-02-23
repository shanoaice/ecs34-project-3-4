#include "StringDataSink.h"
#include "StringDataSource.h"
#include "XMLEntity.h"
#include "XMLReader.h"
#include "XMLWriter.h"
// #include <algorithm>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

std::string reference_xml =
    R"(<?xml version="1.0" encoding="UTF-8"?>
<root>
    <element attribute="value">Content goes here</element>
    <special_characters>
        <less_than>&lt;</less_than>
        <greater_than>&gt;</greater_than>
        <ampersand tag="&amp;">&amp;</ampersand>
        <apostrophe tag="&apos;">'</apostrophe>
        <quote tag="&quot;">"</quote>
    </special_characters>
    <nested_structure>
        <item id="1">First Item</item>
        <item id="2">Second Item</item>
    </nested_structure>
    <empty_element/>
</root>
)";

std::string malformed_xml =
    R"(<?xml version="1.0" encoding="UTF-8"?>
<root>
    <element attribute="value">Content goes here</element>
    <special_characters>
        <less_than>&lt;</less_than>
        <greater_than>&gt;</greater_than>
        <ampe&rsand tag="&amp;">&amp;</ampersand>
        <apostrophe tag="&apos;">'</apostrophe>
        <quote tag="&quot;">"</quote>
    </special_characters>
    <nested_structure>
        <item id="1">First Item</item>
        <item id="2">Second Item</item>
    </nested_structure>
    <empty_element/>
</root>
)";

SXMLEntity root_begin = {SXMLEntity::EType::StartElement, "root", {}};

SXMLEntity regular_element_begin = {
    SXMLEntity::EType::StartElement, "element", {{"attribute", "value"}}};

SXMLEntity regular_element_cdata = {
    SXMLEntity::EType::CharData, "Content goes here", {}};

SXMLEntity regular_element_end = {SXMLEntity::EType::EndElement, "element", {}};

SXMLEntity special_characters_begin = {
    SXMLEntity::EType::StartElement, "special_characters", {}};

SXMLEntity less_than_begin = {SXMLEntity::EType::StartElement, "less_than", {}};

SXMLEntity less_than_cdata = {SXMLEntity::EType::CharData, "<", {}};

SXMLEntity less_than_end = {SXMLEntity::EType::EndElement, "less_than", {}};

SXMLEntity greater_than_begin = {
    SXMLEntity::EType::StartElement, "greater_than", {}};

SXMLEntity greater_than_cdata = {SXMLEntity::EType::CharData, ">", {}};

SXMLEntity greater_than_end = {
    SXMLEntity::EType::EndElement, "greater_than", {}};

SXMLEntity ampersand_begin = {
    SXMLEntity::EType::StartElement, "ampersand", {{"tag", "&"}}};

SXMLEntity ampersand_cdata = {SXMLEntity::EType::CharData, "&", {}};

SXMLEntity ampersand_end = {SXMLEntity::EType::EndElement, "ampersand", {}};

SXMLEntity apostrophe_begin = {
    SXMLEntity::EType::StartElement, "apostrophe", {{"tag", "'"}}};

SXMLEntity apostrophe_cdata = {SXMLEntity::EType::CharData, "'", {}};

SXMLEntity apostrophe_end = {SXMLEntity::EType::EndElement, "apostrophe", {}};

SXMLEntity quote_begin = {
    SXMLEntity::EType::StartElement, "quote", {{"tag", "\""}}};

SXMLEntity quote_cdata = {SXMLEntity::EType::CharData, "\"", {}};

SXMLEntity quote_end = {SXMLEntity::EType::EndElement, "quote", {}};

SXMLEntity special_characters_end = {
    SXMLEntity::EType::EndElement, "special_characters", {}};

SXMLEntity nested_structure_begin = {
    SXMLEntity::EType::StartElement, "nested_structure", {}};

SXMLEntity item_begin = {
    SXMLEntity::EType::StartElement, "item", {{"id", "1"}}};

SXMLEntity item_cdata = {SXMLEntity::EType::CharData, "First Item", {}};

SXMLEntity item_end = {SXMLEntity::EType::EndElement, "item", {}};

SXMLEntity item2_begin = {
    SXMLEntity::EType::StartElement, "item", {{"id", "2"}}};

SXMLEntity item2_cdata = {SXMLEntity::EType::CharData, "Second Item", {}};

SXMLEntity item2_end = {SXMLEntity::EType::EndElement, "item", {}};

SXMLEntity nested_structure_end = {
    SXMLEntity::EType::EndElement,
    "nested_structure",
    {},
};

SXMLEntity empty_element_complete = {
    SXMLEntity::EType::CompleteElement, "empty_element", {}};

SXMLEntity empty_element_begin = {
    SXMLEntity::EType::StartElement, "empty_element", {}};

SXMLEntity empty_element_end = {
    SXMLEntity::EType::EndElement, "empty_element", {}};

SXMLEntity root_end = {SXMLEntity::EType::EndElement, "root", {}};

std::vector<SXMLEntity> reference = {
    root_begin,
    regular_element_begin,
    regular_element_cdata,
    regular_element_end,
    special_characters_begin,
    less_than_begin,
    less_than_cdata,
    less_than_end,
    greater_than_begin,
    greater_than_cdata,
    greater_than_end,
    ampersand_begin,
    ampersand_cdata,
    ampersand_end,
    apostrophe_begin,
    apostrophe_cdata,
    apostrophe_end,
    quote_begin,
    quote_cdata,
    quote_end,
    special_characters_end,
    nested_structure_begin,
    item_begin,
    item_cdata,
    item_end,
    item2_begin,
    item2_cdata,
    item2_end,
    nested_structure_end,
    empty_element_complete,
    root_end,
};

std::vector<SXMLEntity> reference_reader = {
    root_begin,
    regular_element_begin,
    regular_element_cdata,
    regular_element_end,
    special_characters_begin,
    less_than_begin,
    less_than_cdata,
    less_than_end,
    greater_than_begin,
    greater_than_cdata,
    greater_than_end,
    ampersand_begin,
    ampersand_cdata,
    ampersand_end,
    apostrophe_begin,
    apostrophe_cdata,
    apostrophe_end,
    quote_begin,
    quote_cdata,
    quote_end,
    special_characters_end,
    nested_structure_begin,
    item_begin,
    item_cdata,
    item_end,
    item2_begin,
    item2_cdata,
    item2_end,
    nested_structure_end,
    empty_element_begin,
    empty_element_end,
    root_end,
};

std::vector<SXMLEntity> reference_skipcdata = {
    root_begin,
    regular_element_begin,
    regular_element_end,
    special_characters_begin,
    less_than_begin,
    less_than_end,
    greater_than_begin,
    greater_than_end,
    ampersand_begin,
    ampersand_end,
    apostrophe_begin,
    apostrophe_end,
    quote_begin,
    quote_end,
    special_characters_end,
    nested_structure_begin,
    item_begin,
    item_end,
    item2_begin,
    item2_end,
    nested_structure_end,
    empty_element_begin,
    empty_element_end,
    root_end,
};

TEST(XMLReader, Reference) {
    auto Source = std::make_shared<CStringDataSource>(reference_xml);

    CXMLReader Reader(Source);

    std::vector<SXMLEntity> entities;
    SXMLEntity entity;
    while (Reader.ReadEntity(entity)) {
        entities.push_back(entity);
    }

    EXPECT_EQ(entities.size(), reference_reader.size());

    EXPECT_EQ(entities, reference_reader);

    // auto mismatch_from_reference = std::ranges::mismatch(entities,
    // reference_reader);

    // EXPECT_EQ(mismatch_from_reference.in1, entities.end()); // basically this
    // means no mismatch
}

TEST(XMLReader, ReferenceSkipcdata) {
    auto Source = std::make_shared<CStringDataSource>(reference_xml);

    CXMLReader Reader(Source);

    std::vector<SXMLEntity> entities;
    SXMLEntity entity;
    while (Reader.ReadEntity(entity, true)) {
        entities.push_back(entity);
    }

    EXPECT_EQ(entities.size(), reference_skipcdata.size());

    EXPECT_EQ(entities, reference_skipcdata);

    // auto mismatch_from_reference = std::ranges::mismatch(entities,
    // reference_reader);

    // EXPECT_EQ(mismatch_from_reference.in1, entities.end()); // basically this
    // means no mismatch
}

TEST(XMLReader, Malformed) {
    auto Source = std::make_shared<CStringDataSource>(malformed_xml);

    CXMLReader Reader(Source);

    SXMLEntity entity;
    while (Reader.ReadEntity(entity)) {
    }

    EXPECT_FALSE(Reader.ReadEntity(entity));
    EXPECT_FALSE(Reader.End());
}

TEST(XMLWriter, WholeDocument) {
    auto Sink = std::make_shared<CStringDataSink>();
    CXMLWriter Writer(Sink);

    for (const auto &entity : reference) {
        Writer.WriteEntity(entity);
    }

    auto Source = std::make_shared<CStringDataSource>(Sink->String());
    CXMLReader Reader(Source);

    std::vector<SXMLEntity> entities;
    SXMLEntity entity;
    while (Reader.ReadEntity(entity)) {
        entities.push_back(entity);
    }

    EXPECT_EQ(entities.size(), reference_reader.size());

    EXPECT_EQ(entities, reference_reader);
}

TEST(XMLWriter, FlushTags) {
    auto Sink = std::make_shared<CStringDataSink>();
    CXMLWriter Writer(Sink);

    SXMLEntity entity_begin = {SXMLEntity::EType::StartElement, "entity", {}};

    SXMLEntity entity_attributes_begin = {SXMLEntity::EType::StartElement,
                                          "entity_attributes",
                                          {{"tag", "attributes"}}};

    SXMLEntity entity_end = {SXMLEntity::EType::EndElement, "entity", {}};

    SXMLEntity entity_attributes_end = {
        SXMLEntity::EType::EndElement, "entity_attributes", {}};

    std::vector reference_output = {entity_begin, entity_attributes_begin,
                                    entity_attributes_end, entity_end};

    Writer.WriteEntity(entity_begin);
    Writer.WriteEntity(entity_attributes_begin);
    Writer.Flush();

    auto Source = std::make_shared<CStringDataSource>(Sink->String());
    CXMLReader Reader(Source);

    std::vector<SXMLEntity> entities;
    SXMLEntity entity;
    while (Reader.ReadEntity(entity)) {
        entities.push_back(entity);
    }

    EXPECT_EQ(reference_output.size(), entities.size());
    EXPECT_EQ(reference_output, entities);
}

TEST(XMLWriter, RejectMalformedEntities) {
    auto Sink = std::make_shared<CStringDataSink>();
    CXMLWriter Writer(Sink);

    SXMLEntity malformed_begin_entity = {
        SXMLEntity::EType::StartElement, "", {}};

    SXMLEntity malformed_end_entity = {SXMLEntity::EType::EndElement, "", {}};

    SXMLEntity malformed_complete_entity = {
        SXMLEntity::EType::CompleteElement, "", {}};

    SXMLEntity malformed_entity_type = {(SXMLEntity::EType)4, "malformed", {}};

    EXPECT_FALSE(Writer.WriteEntity(malformed_begin_entity));
    EXPECT_FALSE(Writer.WriteEntity(malformed_end_entity));
    EXPECT_FALSE(Writer.WriteEntity(malformed_complete_entity));
    EXPECT_FALSE(Writer.WriteEntity(malformed_entity_type));
}

TEST(XMLWriter, RejectMismatchedTags) {
    auto Sink = std::make_shared<CStringDataSink>();
    CXMLWriter Writer(Sink);

    SXMLEntity mismatched_begin_entity = {
        SXMLEntity::EType::StartElement, "mismatched", {}};

    SXMLEntity mismatched_end_entity = {
        SXMLEntity::EType::EndElement, "mismatch", {}};

    Writer.WriteEntity(mismatched_begin_entity);

    EXPECT_FALSE(Writer.WriteEntity(mismatched_end_entity));
}
