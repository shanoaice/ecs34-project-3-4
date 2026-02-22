#include <gtest/gtest.h>
#include "CSVBusSystem.h"
#include "StringDataSource.h"
#include "DSVReader.h"

TEST(CSVBusSystem, StopTest){
    auto StopDataSource = std::make_shared< CStringDataSource >("stop_id,node_id\n"
                                                                "1,123\n"
                                                                "2,124");
    auto StopReader = std::make_shared< CDSVReader >(StopDataSource,',');
    auto RouteReader = std::make_shared< CDSVReader >(std::make_shared<CStringDataSource>("route,stop_id"),',');
    
    CCSVBusSystem BusSystem(StopReader, RouteReader);
    
    // test stop count
    EXPECT_EQ(BusSystem.StopCount(),2);
    
    // test stop by index
    auto StopObj = BusSystem.StopByIndex(0);
    EXPECT_NE(StopObj,nullptr);
    EXPECT_EQ(StopObj->ID(), 1);
    EXPECT_EQ(StopObj->NodeID(), 123);

    StopObj = BusSystem.StopByIndex(1);
    EXPECT_NE(StopObj,nullptr);
    EXPECT_EQ(StopObj->ID(), 2);
    EXPECT_EQ(StopObj->NodeID(), 124);

    // exceed boundary
    auto OutStopObj = BusSystem.StopByIndex(1111);
    EXPECT_EQ(OutStopObj, nullptr);
    
    // stop by id
    StopObj = BusSystem.StopByID(1);
    ASSERT_NE(StopObj,nullptr);
    EXPECT_EQ(StopObj->ID(),1);
    EXPECT_EQ(StopObj->NodeID(),123);
    
    StopObj = BusSystem.StopByID(2);
    ASSERT_NE(StopObj,nullptr);
    EXPECT_EQ(StopObj->ID(),2);
    EXPECT_EQ(StopObj->NodeID(),124);
    
    // not exist
    auto NotExistStop = BusSystem.StopByID(11111);
    EXPECT_EQ(NotExistStop, nullptr);
}

TEST(CSVBusSystem, RouteTest){
    auto StopDataSource = std::make_shared< CStringDataSource >("stop_id,node_id\n"
                                                                "1,123\n"
                                                                "2,124");
    auto StopReader = std::make_shared< CDSVReader >(StopDataSource,',');
    auto RouteDataSource = std::make_shared< CStringDataSource >("route,stop_id\n"
                                                                "A,1\n"
                                                                "A,2");
    auto RouteReader = std::make_shared< CDSVReader >(RouteDataSource,',');
    
    CCSVBusSystem BusSystem(StopReader,RouteReader);
    
    // test count
    EXPECT_EQ(BusSystem.RouteCount(), 1);

    // route by index
    auto RouteObj = BusSystem.RouteByIndex(0);
    EXPECT_NE(RouteObj, nullptr);
    EXPECT_EQ(RouteObj->Name(), "A");
    EXPECT_EQ(RouteObj->StopCount(), 2);
    EXPECT_EQ(RouteObj->GetStopID(0), 1);
    EXPECT_EQ(RouteObj->GetStopID(1), 2);
    // exceed boundary
    EXPECT_EQ(RouteObj->GetStopID(2), CBusSystem::InvalidStopID);
    
    // exceed boundary
    auto OutRouteObj = BusSystem.RouteByIndex(1);
    EXPECT_EQ(OutRouteObj, nullptr);

    // route by name
    RouteObj = BusSystem.RouteByName("A");
    EXPECT_NE(RouteObj, nullptr);
    EXPECT_EQ(RouteObj->StopCount(), 2);
    
    // not exist
    auto NotExistRouteObj = BusSystem.RouteByName("B");
    EXPECT_EQ(NotExistRouteObj, nullptr);
    
    // not exist
    NotExistRouteObj = BusSystem.RouteByName("");
    EXPECT_EQ(BusSystem.RouteByName(""), nullptr);
}

// more than one route
TEST(CSVBusSystem, MoreRouteTest){
    auto StopDataSource = std::make_shared< CStringDataSource >("stop_id,node_id\n"
                                                                "1,123\n"
                                                                "2,124");
    auto StopReader = std::make_shared< CDSVReader >(StopDataSource, ',');

    auto RouteDataSource = std::make_shared< CStringDataSource >("route,stop_id\n"
                                                                 "B,1\n"
                                                                 "A,2");
    auto RouteReader = std::make_shared< CDSVReader >(RouteDataSource, ',');
    CCSVBusSystem BusSystem(StopReader, RouteReader);
    
    // route count
    EXPECT_EQ(BusSystem.RouteCount(), 2);
    
    // order of route
    auto First = BusSystem.RouteByIndex(0);
    auto Second = BusSystem.RouteByIndex(1);
    ASSERT_NE(First, nullptr);
    ASSERT_NE(Second, nullptr);
    EXPECT_EQ(First->Name(), "B");
    EXPECT_EQ(Second->Name(), "A");
}


// when route has repeated stop
TEST(CSVBusSystem, RepeatedStopTest){
    auto StopDataSource = std::make_shared< CStringDataSource >("stop_id,node_id\n"
                                                                "1,123\n"
                                                                "2,124");
    auto StopReader = std::make_shared< CDSVReader >(StopDataSource, ',');

    auto RouteDataSource = std::make_shared< CStringDataSource >("route,stop_id\n"
                                                                 "A,1\n"
                                                                 "A,2\n"
                                                                 "A,1"); 
    auto RouteReader = std::make_shared< CDSVReader >(RouteDataSource, ',');
    CCSVBusSystem BusSystem(StopReader, RouteReader);
    
    auto RouteObj = BusSystem.RouteByName("A");
    ASSERT_NE(RouteObj, nullptr);
    
    // stop count
    EXPECT_EQ(RouteObj->StopCount(), 3); 
    
    // stop order
    EXPECT_EQ(RouteObj->GetStopID(0), 1);
    EXPECT_EQ(RouteObj->GetStopID(1), 2);
    EXPECT_EQ(RouteObj->GetStopID(2), 1);
}