#include "CSVBusSystem.h"
#include "DSVReader.h"
#include "StringDataSource.h"
#include "XMLReader.h"
#include <gtest/gtest.h>

TEST(CSVBusSystem, SimpleTest) {
    auto InStreamStops = std::make_shared<CStringDataSource>("stop_id,node_id");
    auto InStreamRoutes = std::make_shared<CStringDataSource>("route,stop_id");
    const auto CSVReaderStops =
        std::make_shared<CDSVReader>(InStreamStops, ',');
    const auto CSVReaderRoutes =
        std::make_shared<CDSVReader>(InStreamRoutes, ',');
    const CCSVBusSystem BusSystem(CSVReaderStops, CSVReaderRoutes);
    EXPECT_EQ(BusSystem.StopCount(), 0);
    EXPECT_EQ(BusSystem.RouteCount(), 0);
}

TEST(CSVBusSystem, StopTest) {
    auto InStreamStops = std::make_shared<CStringDataSource>("stop_id,node_id\n"
                                                             "1,101\n"
                                                             "2,102");
    auto InStreamRoutes = std::make_shared<CStringDataSource>("route,stop_id");
    auto CSVReaderStops = std::make_shared<CDSVReader>(InStreamStops, ',');
    auto CSVReaderRoutes = std::make_shared<CDSVReader>(InStreamRoutes, ',');
    const CCSVBusSystem BusSystem(CSVReaderStops, CSVReaderRoutes);
    EXPECT_EQ(BusSystem.StopCount(), 2);
    EXPECT_EQ(BusSystem.RouteCount(), 0);
    auto Stop1Index = BusSystem.StopByIndex(0);
    auto Stop1ID = BusSystem.StopByID(1);
    EXPECT_EQ(Stop1Index, Stop1ID);
    ASSERT_TRUE(static_cast<bool>(Stop1Index));
    EXPECT_EQ(Stop1Index->ID(), 1);
    EXPECT_EQ(Stop1Index->NodeID(), 101);
    auto Stop2Index = BusSystem.StopByIndex(1);
    auto Stop2ID = BusSystem.StopByID(2);
    EXPECT_EQ(Stop2Index, Stop2ID);
    ASSERT_TRUE(static_cast<bool>(Stop2Index));
    EXPECT_EQ(Stop2Index->ID(), 2);
    EXPECT_EQ(Stop2Index->NodeID(), 102);
}

TEST(CSVBusSystem, RouteTest) {
    auto InStreamStops = std::make_shared<CStringDataSource>("stop_id,node_id\n"
                                                             "1,101\n"
                                                             "2,102");
    auto InStreamRoutes = std::make_shared<CStringDataSource>("route,stop_id\n"
                                                              "A,1\n"
                                                              "A,2\n"
                                                              "A,1");
    const auto CSVReaderStops =
        std::make_shared<CDSVReader>(InStreamStops, ',');
    const std::shared_ptr<CDSVReader> CSVReaderRoutes =
        std::make_shared<CDSVReader>(InStreamRoutes, ',');
    const CCSVBusSystem BusSystem(CSVReaderStops, CSVReaderRoutes);
    EXPECT_EQ(BusSystem.StopCount(), 2);
    EXPECT_EQ(BusSystem.RouteCount(), 1);
    const auto Route1Index = BusSystem.RouteByIndex(0);
    const auto Route1ID = BusSystem.RouteByName("A");
    EXPECT_EQ(Route1Index, Route1ID);
    ASSERT_TRUE(static_cast<bool>(Route1Index));
    EXPECT_EQ(Route1Index->Name(), "A");
    EXPECT_EQ(Route1Index->StopCount(), 3);
    EXPECT_EQ(Route1Index->GetStopID(0), 1);
    EXPECT_EQ(Route1Index->GetStopID(1), 2);
    EXPECT_EQ(Route1Index->GetStopID(2), 1);
}
