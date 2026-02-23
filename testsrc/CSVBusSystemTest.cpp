#include "CSVBusSystem.h"
#include "DSVReader.h"
#include "StringDataSource.h"

#include <fstream>
#include <gtest/gtest.h>
#include <memory>

class CSVBusSystemTest : public ::testing::Test {
  protected:
    std::shared_ptr<CDSVReader> StopReader, RouteReader;
    CSVBusSystemTest() {
        std::ifstream routeDataFile, stopDataFile;
        routeDataFile.open("./data/routes.csv");
        stopDataFile.open("./data/stops.csv");

        auto routeDataSource = std::make_shared<CStringDataSource>(
            std::string(std::istreambuf_iterator<char>(routeDataFile),
                        std::istreambuf_iterator<char>()));
        auto stopDataSource = std::make_shared<CStringDataSource>(
            std::string(std::istreambuf_iterator<char>(stopDataFile),
                        std::istreambuf_iterator<char>()));

        StopReader = std::make_shared<CDSVReader>(stopDataSource, ',');
        RouteReader = std::make_shared<CDSVReader>(routeDataSource, ',');
    }
};

TEST_F(CSVBusSystemTest, StopTest) {
    CCSVBusSystem BusSystem(StopReader, RouteReader);

    // test stop count
    EXPECT_EQ(BusSystem.StopCount(), 298);

    // test stop by index
    auto StopObj = BusSystem.StopByIndex(0);
    EXPECT_NE(StopObj, nullptr);
    EXPECT_EQ(StopObj->ID(), 22043ull);
    EXPECT_EQ(StopObj->NodeID(), 2849810514ull);

    StopObj = BusSystem.StopByIndex(5);
    EXPECT_NE(StopObj, nullptr);
    EXPECT_EQ(StopObj->ID(), 22177ull);
    EXPECT_EQ(StopObj->NodeID(), 2599137935ull);

    // exceed boundary
    auto OutStopObj = BusSystem.StopByIndex(BusSystem.StopCount());
    EXPECT_EQ(OutStopObj, nullptr);

    // stop by id
    StopObj = BusSystem.StopByID(22173ull);
    ASSERT_NE(StopObj, nullptr);
    EXPECT_EQ(StopObj->ID(), 22173ull);
    EXPECT_EQ(StopObj->NodeID(), 2276473527ull);

    StopObj = BusSystem.StopByID(807737ull);
    ASSERT_NE(StopObj, nullptr);
    EXPECT_EQ(StopObj->ID(), 807737ull);
    EXPECT_EQ(StopObj->NodeID(), 2283899823ull);

    // not exist
    auto NotExistStop = BusSystem.StopByID(19928304ull);
    EXPECT_EQ(NotExistStop, nullptr);
}

TEST_F(CSVBusSystemTest, RouteTest) {
    const CCSVBusSystem BusSystem(StopReader, RouteReader);

    // test count
    EXPECT_EQ(BusSystem.RouteCount(), 17);

    // route by index
    auto RouteObj = BusSystem.RouteByIndex(0);
    EXPECT_NE(RouteObj, nullptr);
    EXPECT_EQ(RouteObj->Name(), "A");
    EXPECT_EQ(RouteObj->StopCount(), 22);
    EXPECT_EQ(RouteObj->GetStopID(0), 22258);
    EXPECT_EQ(RouteObj->GetStopID(1), 22169);
    // exceed boundary
    EXPECT_EQ(RouteObj->GetStopID(23), CBusSystem::InvalidStopID);

    // exceed boundary
    const auto OutRouteObj = BusSystem.RouteByIndex(BusSystem.RouteCount());
    EXPECT_EQ(OutRouteObj, nullptr);

    // route by name
    RouteObj = BusSystem.RouteByName("A");
    EXPECT_NE(RouteObj, nullptr);
    EXPECT_EQ(RouteObj->StopCount(), 22);

    // route by index
    RouteObj = BusSystem.RouteByName("D");
    EXPECT_NE(RouteObj, nullptr);
    EXPECT_EQ(RouteObj->Name(), "D");
    EXPECT_EQ(RouteObj->StopCount(), 17);
    EXPECT_EQ(RouteObj->GetStopID(0), 22256);
    EXPECT_EQ(RouteObj->GetStopID(1), 22239);
    // exceed boundary
    EXPECT_EQ(RouteObj->GetStopID(17), CBusSystem::InvalidStopID);

    // not exist
    auto NotExistRouteObj = BusSystem.RouteByName("S");
    EXPECT_EQ(NotExistRouteObj, nullptr);

    // not exist
    NotExistRouteObj = BusSystem.RouteByName("");
    EXPECT_EQ(NotExistRouteObj, nullptr);
}

// more than one route
TEST(CSVBusSystemTestHandwritten, MoreRouteTest) {
    auto StopDataSource =
        std::make_shared<CStringDataSource>("stop_id,node_id\n"
                                            "1,123\n"
                                            "2,124");
    const auto StopReader = std::make_shared<CDSVReader>(StopDataSource, ',');

    auto RouteDataSource = std::make_shared<CStringDataSource>("route,stop_id\n"
                                                               "B,1\n"
                                                               "A,2");
    const auto RouteReader = std::make_shared<CDSVReader>(RouteDataSource, ',');
    const CCSVBusSystem BusSystem(StopReader, RouteReader);

    // route count
    EXPECT_EQ(BusSystem.RouteCount(), 2);

    // order of route
    const auto First = BusSystem.RouteByIndex(0);
    const auto Second = BusSystem.RouteByIndex(1);
    ASSERT_NE(First, nullptr);
    ASSERT_NE(Second, nullptr);
    EXPECT_EQ(First->Name(), "B");
    EXPECT_EQ(Second->Name(), "A");
}

// when route has repeated stop
TEST(CSVBusSystemTestHandwritten, RepeatedStopTest) {
    auto StopDataSource =
        std::make_shared<CStringDataSource>("stop_id,node_id\n"
                                            "1,123\n"
                                            "2,124");
    const auto StopReader = std::make_shared<CDSVReader>(StopDataSource, ',');

    auto RouteDataSource = std::make_shared<CStringDataSource>("route,stop_id\n"
                                                               "A,1\n"
                                                               "A,2\n"
                                                               "A,1");
    const auto RouteReader = std::make_shared<CDSVReader>(RouteDataSource, ',');
    const CCSVBusSystem BusSystem(StopReader, RouteReader);

    const auto RouteObj = BusSystem.RouteByName("A");
    ASSERT_NE(RouteObj, nullptr);

    // stop count
    EXPECT_EQ(RouteObj->StopCount(), 3);

    // stop order
    EXPECT_EQ(RouteObj->GetStopID(0), 1);
    EXPECT_EQ(RouteObj->GetStopID(1), 2);
    EXPECT_EQ(RouteObj->GetStopID(2), 1);
}
