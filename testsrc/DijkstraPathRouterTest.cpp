#include <gtest/gtest.h>
#include "DijkstraPathRouter.h"

TEST(DijkstraPathRouter, SimpleTest){
    CDijkstraPathRouter PathRouter;

    EXPECT_EQ(PathRouter.VertexCount(),0);
    auto VertexID = PathRouter.AddVertex(std::string("foo"));
    EXPECT_EQ(PathRouter.VertexCount(),1);
    auto VetexTag = std::any_cast<std::string>(PathRouter.GetVertexTag(VertexID));
    EXPECT_EQ(VetexTag,"foo");
    
}

TEST(DijkstraPathRouter, ShortestPath){
    CDijkstraPathRouter PathRouter;
    /*
                5   
       A ---> B --->C
          4  2|  ___^
              V / 1
              D
    
    */
    auto vA = PathRouter.AddVertex(std::string("A"));
    auto vB = PathRouter.AddVertex(std::string("B"));
    auto vC = PathRouter.AddVertex(std::string("C"));
    auto vD = PathRouter.AddVertex(std::string("D"));
    EXPECT_EQ(PathRouter.VertexCount(),4);
    EXPECT_TRUE(PathRouter.AddEdge(vA,vB,4.0));
    EXPECT_TRUE(PathRouter.AddEdge(vB,vC,5.0));
    EXPECT_TRUE(PathRouter.AddEdge(vB,vD,2.0));
    EXPECT_TRUE(PathRouter.AddEdge(vD,vC,1.0));
    
    //A -> B -> D -> C: 4 + 2 + 1 = 7 shortest path
    std::vector<CPathRouter::TVertexID> Path;
    EXPECT_EQ(PathRouter.FindShortestPath(vA,vC,Path), 7.0);
    std::vector<CPathRouter::TVertexID> ExpectedPath{vA,vB,vD,vC};
    EXPECT_EQ(Path,ExpectedPath);
    
}


TEST(DijkstraPathRouter, NotConnected){
    CDijkstraPathRouter PathRouter;
    /*
          10         
       E ---> F     G
    */
    auto vE = PathRouter.AddVertex(std::string("E"));
    auto vF = PathRouter.AddVertex(std::string("F"));
    auto vG = PathRouter.AddVertex(std::string("G"));
    PathRouter.AddEdge(vE, vF, 10.0);

    std::vector<CPathRouter::TVertexID> Path;
    EXPECT_EQ(PathRouter.FindShortestPath(vE, vG, Path), CPathRouter::NoPathExists);
    EXPECT_TRUE(Path.empty());
}

TEST(DijkstraPathRouter, NegativeWeight){
    CDijkstraPathRouter PathRouter;
    /*        
        I -> J
          -5       Error!(Dijkstra cannot do negative edge)
    */
    auto vI = PathRouter.AddVertex(std::string("I"));
    auto vJ = PathRouter.AddVertex(std::string("J"));
    EXPECT_FALSE(PathRouter.AddEdge(vI, vJ, -5.0));
}

TEST(DijkstraPathRouter, SameSrcDest){
    CDijkstraPathRouter PathRouter;
    /*  0      
        H -> 
        ^    | 10          // self loop, but shortest should be 0
        | <- v

    */
    auto vH = PathRouter.AddVertex(std::string("H"));
    PathRouter.AddEdge(vH, vH, 10.0);
    std::vector<CPathRouter::TVertexID> Path;
    EXPECT_DOUBLE_EQ(PathRouter.FindShortestPath(vH, vH, Path), 0.0);
    std::vector<CPathRouter::TVertexID> ExpectedPath{vH};
    EXPECT_EQ(Path, ExpectedPath);
}