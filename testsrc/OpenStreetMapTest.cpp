#include "OpenStreetMap.h"
#include "StringDataSource.h"
#include "XMLReader.h"
#include <fstream>
#include <gtest/gtest.h>

class OpenStreetMapTest : public ::testing::Test {
  protected:
    OpenStreetMapTest() {
        std::ifstream osmDataFile;
        osmDataFile.open("./data/davis.osm");
        std::string osmData{std::istreambuf_iterator<char>(osmDataFile),
                            std::istreambuf_iterator<char>()};
        osmDataFile.close();

        dataSource = std::make_shared<CStringDataSource>(osmData);
        reader = std::make_shared<CXMLReader>(dataSource);
        osm = std::make_unique<COpenStreetMap>(reader);
    }

    std::shared_ptr<CStringDataSource> dataSource;
    std::shared_ptr<CXMLReader> reader;
    std::unique_ptr<COpenStreetMap> osm;

    ~OpenStreetMapTest() override = default;
};

TEST_F(OpenStreetMapTest, NodeCount) {
    EXPECT_EQ(osm->NodeCount(), 10259);
}

TEST_F(OpenStreetMapTest, WayCount) {
    EXPECT_EQ(osm->WayCount(), 1644);
}

TEST_F(OpenStreetMapTest, NodeByIndex) {
    // simple node
    const auto node0 = osm->NodeByIndex(0);
    constexpr std::pair<double, double> node0_location{38.5178523,
                                                       -121.7712408};
    EXPECT_EQ(node0->ID(), 62208369ull);

    EXPECT_EQ(node0->Location(), node0_location);

    // complex node
    const auto node41 = osm->NodeByIndex(41);
    constexpr std::pair<double, double> node41_location{38.5222679,
                                                        -121.7673869};
    EXPECT_EQ(node41->ID(), 62224641ull);

    EXPECT_EQ(node41->Location(), node41_location);

    EXPECT_EQ(node41->AttributeCount(), 2);

    EXPECT_TRUE(node41->HasAttribute("highway"));
    EXPECT_TRUE(node41->HasAttribute("ref"));

    EXPECT_EQ(node41->GetAttributeKey(0), "highway");
    EXPECT_EQ(node41->GetAttributeKey(1), "ref");

    EXPECT_EQ(node41->GetAttributeKey(2), "");

    EXPECT_EQ(node41->GetAttribute("highway"), "motorway_junction");
    EXPECT_EQ(node41->GetAttribute("ref"), "71");

    EXPECT_EQ(node41->GetAttribute("invalid"), "");

    // idx >= nodeCount
    EXPECT_EQ(osm->NodeByIndex(osm->NodeCount()), nullptr);
}

TEST_F(OpenStreetMapTest, NodeByID) {
    // simple node
    const auto node_id62345194 = osm->NodeByID(62345194ull);
    constexpr std::pair<double, double> node62345194_location{38.5199603,
                                                              -121.7692337};
    EXPECT_EQ(node_id62345194->ID(), 62345194ull);

    EXPECT_EQ(node_id62345194->Location(), node62345194_location);

    // complex node
    const auto node_id62224641 = osm->NodeByID(62224641ull);
    constexpr std::pair<double, double> node62224641_location{38.5222679,
                                                              -121.7673869};
    EXPECT_EQ(node_id62224641->ID(), 62224641ull);

    EXPECT_EQ(node_id62224641->Location(), node62224641_location);

    EXPECT_EQ(node_id62224641->AttributeCount(), 2);

    EXPECT_TRUE(node_id62224641->HasAttribute("highway"));
    EXPECT_TRUE(node_id62224641->HasAttribute("ref"));

    EXPECT_EQ(node_id62224641->GetAttributeKey(0), "highway");
    EXPECT_EQ(node_id62224641->GetAttributeKey(1), "ref");

    EXPECT_EQ(node_id62224641->GetAttributeKey(2), "");

    EXPECT_EQ(node_id62224641->GetAttribute("highway"), "motorway_junction");
    EXPECT_EQ(node_id62224641->GetAttribute("ref"), "71");

    EXPECT_EQ(node_id62224641->GetAttribute("invalid"), "");

    // non-existent node ID
    EXPECT_EQ(osm->NodeByID(23849999ull), nullptr);
}

TEST_F(OpenStreetMapTest, WayByIndex) {
    // way 1: motorway_link I-80 E to Sac
    const auto way0 = osm->WayByIndex(0);
    EXPECT_EQ(way0->ID(), 8699536ull);

    EXPECT_EQ(way0->NodeCount(), 10);

    EXPECT_EQ(way0->GetNodeID(0), 258592863ull);
    // idx >= nodeCount
    EXPECT_EQ(way0->GetNodeID(way0->NodeCount()), CStreetMap::InvalidNodeID);

    EXPECT_EQ(way0->AttributeCount(), 6);

    EXPECT_EQ(way0->GetAttributeKey(0), "lanes");
    EXPECT_EQ(way0->GetAttributeKey(1), "oneway");
    EXPECT_EQ(way0->GetAttributeKey(2), "bicycle");
    EXPECT_EQ(way0->GetAttributeKey(3), "highway");
    EXPECT_EQ(way0->GetAttributeKey(4), "destination");
    EXPECT_EQ(way0->GetAttributeKey(5), "destination:ref");

    EXPECT_EQ(way0->GetAttributeKey(6), "");

    EXPECT_TRUE(way0->HasAttribute("lanes"));
    EXPECT_TRUE(way0->HasAttribute("oneway"));
    EXPECT_TRUE(way0->HasAttribute("bicycle"));
    EXPECT_TRUE(way0->HasAttribute("highway"));
    EXPECT_TRUE(way0->HasAttribute("destination"));
    EXPECT_TRUE(way0->HasAttribute("destination:ref"));

    EXPECT_FALSE(way0->HasAttribute("invalid"));

    EXPECT_EQ(way0->GetAttribute("lanes"), "2");
    EXPECT_EQ(way0->GetAttribute("oneway"), "yes");
    EXPECT_EQ(way0->GetAttribute("bicycle"), "no");
    EXPECT_EQ(way0->GetAttribute("highway"), "motorway_link");
    EXPECT_EQ(way0->GetAttribute("destination"), "Sacramento");
    EXPECT_EQ(way0->GetAttribute("destination:ref"), "I 80 East");

    EXPECT_EQ(way0->GetAttribute("invalid"), "");

    const auto way4 = osm->WayByIndex(4);
    EXPECT_EQ(way4->ID(), 8706922ull);

    EXPECT_EQ(way4->NodeCount(), 3);

    EXPECT_EQ(way4->GetNodeID(0), 62321892ull);
    EXPECT_EQ(way4->GetNodeID(way4->NodeCount()), CStreetMap::InvalidNodeID);

    EXPECT_EQ(way4->AttributeCount(), 1);

    EXPECT_EQ(way4->GetAttributeKey(0), "highway");

    EXPECT_EQ(way4->GetAttributeKey(1), "");

    EXPECT_TRUE(way4->HasAttribute("highway"));

    EXPECT_FALSE(way4->HasAttribute("invalid"));

    EXPECT_EQ(way4->GetAttribute("highway"), "residential");

    EXPECT_EQ(way4->GetAttribute("invalid"), "");

    // idx >= wayCount
    EXPECT_EQ(osm->WayByIndex(osm->WayCount()), nullptr);
}

TEST_F(OpenStreetMapTest, WayByID) {
    // way 1: motorway_link I-80 E to Sac
    const auto way_id8699536 = osm->WayByID(8699536ull);
    EXPECT_EQ(way_id8699536->ID(), 8699536ull);

    EXPECT_EQ(way_id8699536->NodeCount(), 10);

    EXPECT_EQ(way_id8699536->GetNodeID(0), 258592863ull);
    // idx >= nodeCount
    EXPECT_EQ(way_id8699536->GetNodeID(way_id8699536->NodeCount()), CStreetMap::InvalidNodeID);

    EXPECT_EQ(way_id8699536->AttributeCount(), 6);

    EXPECT_EQ(way_id8699536->GetAttributeKey(0), "lanes");
    EXPECT_EQ(way_id8699536->GetAttributeKey(1), "oneway");
    EXPECT_EQ(way_id8699536->GetAttributeKey(2), "bicycle");
    EXPECT_EQ(way_id8699536->GetAttributeKey(3), "highway");
    EXPECT_EQ(way_id8699536->GetAttributeKey(4), "destination");
    EXPECT_EQ(way_id8699536->GetAttributeKey(5), "destination:ref");

    EXPECT_EQ(way_id8699536->GetAttributeKey(6), "");

    EXPECT_TRUE(way_id8699536->HasAttribute("lanes"));
    EXPECT_TRUE(way_id8699536->HasAttribute("oneway"));
    EXPECT_TRUE(way_id8699536->HasAttribute("bicycle"));
    EXPECT_TRUE(way_id8699536->HasAttribute("highway"));
    EXPECT_TRUE(way_id8699536->HasAttribute("destination"));
    EXPECT_TRUE(way_id8699536->HasAttribute("destination:ref"));

    EXPECT_FALSE(way_id8699536->HasAttribute("invalid"));

    EXPECT_EQ(way_id8699536->GetAttribute("lanes"), "2");
    EXPECT_EQ(way_id8699536->GetAttribute("oneway"), "yes");
    EXPECT_EQ(way_id8699536->GetAttribute("bicycle"), "no");
    EXPECT_EQ(way_id8699536->GetAttribute("highway"), "motorway_link");
    EXPECT_EQ(way_id8699536->GetAttribute("destination"), "Sacramento");
    EXPECT_EQ(way_id8699536->GetAttribute("destination:ref"), "I 80 East");

    EXPECT_EQ(way_id8699536->GetAttribute("invalid"), "");

    const auto way_id8706922 = osm->WayByID(8706922ull);
    EXPECT_EQ(way_id8706922->ID(), 8706922ull);

    EXPECT_EQ(way_id8706922->NodeCount(), 3);

    EXPECT_EQ(way_id8706922->GetNodeID(0), 62321892ull);
    EXPECT_EQ(way_id8706922->GetNodeID(way_id8706922->NodeCount()), CStreetMap::InvalidNodeID);

    EXPECT_EQ(way_id8706922->AttributeCount(), 1);

    EXPECT_EQ(way_id8706922->GetAttributeKey(0), "highway");

    EXPECT_EQ(way_id8706922->GetAttributeKey(1), "");

    EXPECT_TRUE(way_id8706922->HasAttribute("highway"));

    EXPECT_FALSE(way_id8706922->HasAttribute("invalid"));

    EXPECT_EQ(way_id8706922->GetAttribute("highway"), "residential");

    EXPECT_EQ(way_id8706922->GetAttribute("invalid"), "");

    // non-existent way id
    EXPECT_EQ(osm->WayByID(92831452ull), nullptr);
}
