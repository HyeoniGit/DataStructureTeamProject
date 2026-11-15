// 실제 팀 BST/Station/Trip/LinkedList를 사용하는 Graph 통합 테스트
// g++ -std=c++17 -Wall -Wextra -Iinclude tests/test_graph.cpp src/modules/StationIndex.cpp -o graph_test
#include <cmath>
#include <iostream>
#include <set>
#include <string>
#include "modules/Graph.h"

static int passed=0, failed=0;
static void check(bool ok, const char* label) {
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << label << '\n';
    if (ok) ++passed; else ++failed;
}
static Station station(const std::string& id) {
    Station s; s.id=id; s.name="Sample "+id; return s;
}
static Trip trip(const std::string& from, const std::string& to, int minutes) {
    Trip t; t.rentStationId=from; t.returnStationId=to; t.useMinutes=minutes; return t;
}
int main() {
    DynamicArray<Station> stations;
    for (const char* id : {"00001","00002","00003","00004"})
        stations.pushBack(station(id));
    StationTree tree;
    check(buildStationTree(stations,tree),"buildStationTree with actual StationIndex");
    check(tree.size()==4,"4 stations inserted into BST");
    LinkedList<Trip> trips;
    trips.pushBack(trip("00001","00002",10));
    trips.pushBack(trip("00001","00002",20));
    trips.pushBack(trip("00002","00001",5));
    trips.pushBack(trip("00001","00003",15));
    trips.pushBack(trip("00003","00004",0));
    trips.pushBack(trip("00001","00001",7));
    trips.pushBack(trip("99999","00001",12));
    trips.pushBack(trip("","00002",8));
    trips.pushBack(trip("00001","00002",-1));
    Graph graph(tree); Graph::BuildStats stats;
    check(!graph.isBuilt(),"initially not built");
    check(graph.buildGraph(trips,stats)==Graph::BuildSuccess,"first build succeeds");
    check(graph.isBuilt(),"isBuilt after build");
    check(stats.added==5 && stats.updated==1 && stats.missingStation==1 && stats.invalidTrip==2,
          "stats: added 5 / updated 1 / missing 1 / invalid 2");
    check(stats.added+stats.updated+stats.missingStation+stats.invalidTrip==9,"all 9 trips accounted for");
    auto* ab=graph.getEdge("00001","00002");
    check(ab && ab->tripCount==2 && ab->totalMinutes==30,"A->B accumulates 2 trips and 30 minutes");
    check(ab && std::fabs(Graph::averageMinutes(ab)-15.0)<1e-9,"average A->B = 15 minutes");
    check(graph.getEdge("00003","00004") && Graph::averageMinutes(graph.getEdge("00003","00004"))==0.0,
          "zero-minute trip is valid");
    check(graph.getEdge("00001","00001")!=nullptr,"self-loop is retained");
    check(graph.getEdge("00004","00001")==nullptr,"absent edge returns nullptr");
    check(graph.getEdge("99999","00001")==nullptr,"absent source returns nullptr");
    check(graph.containsStation("00004") && !graph.containsStation("99999"),"containsStation uses BST");
    check(graph.isOneWay("00001","00003"),"A->C is one-way");
    check(!graph.isOneWay("00001","00002"),"A<->B is not one-way");
    check(!graph.isOneWay("00001","00001"),"self-loop is not one-way");
    check(!graph.isOneWay("99999","00001"),"unknown station is not one-way");
    auto* a=tree.find("00001"); auto* b=tree.find("00002");
    auto* c=tree.find("00003"); auto* d=tree.find("00004");
    check(a && a->outDegree==3 && a->inDegree==2 && a->rentCount==4 && a->returnCount==2,
          "A: degrees 3/2 and counters 4/2 (self-loop included)");
    check(b && b->outDegree==1 && b->inDegree==1 && b->rentCount==1 && b->returnCount==2,
          "B: degrees 1/1 and counters 1/2");
    check(c && c->outDegree==1 && c->inDegree==1 && c->rentCount==1 && c->returnCount==1,
          "C: degrees 1/1 and counters 1/1");
    check(d && d->outDegree==0 && d->inDegree==1 && d->rentCount==0 && d->returnCount==1,
          "D: degrees 0/1 and counters 0/1");
    std::set<std::string> neighbors;
    graph.forEachNeighbor("00001",[&](const std::string& id){neighbors.insert(id);});
    check(neighbors==std::set<std::string>({"00001","00002","00003"}),"BFS neighbor callback yields A/B/C");
    int calls=0;
    graph.forEachNeighbor("99999",[&](const std::string&){++calls;});
    graph.forEachNeighbor("00004",[&](const std::string&){++calls;});
    check(calls==0,"no callbacks for missing station or empty neighbors");
    Graph::BuildStats again;
    check(graph.buildGraph(trips,again)==Graph::AlreadyBuilt,"second build returns AlreadyBuilt");
    check(again.added==stats.added && again.updated==stats.updated,"second build returns stored stats");
    check(graph.getEdge("00001","00002")->tripCount==2,"second build does not duplicate edges");
    check(Graph::averageMinutes(nullptr)==0.0,"averageMinutes(nullptr) safe");
    std::cout << "RESULT: " << passed << " passed, " << failed << " failed\n";
    return failed ? 1 : 0;
}
