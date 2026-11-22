// BFS (Traversal) 테스트 — 실제 팀 BST/Graph/Queue 사용
// make test  또는
// g++ -std=c++17 -Wall -Wextra -Iinclude tests/test_traversal.cpp src/modules/*.cpp -o traversal_test
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <string>

#include "modules/Traversal.h"

static int passed = 0, failed = 0;
static void check(bool ok, const char* label) {
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << label << '\n';
    if (ok) ++passed; else ++failed;
}
static Station station(const std::string& id) {
    Station s; s.id = id; s.name = "S" + id; return s;
}
static Trip trip(const std::string& from, const std::string& to) {
    Trip t; t.rentStationId = from; t.returnStationId = to; t.useMinutes = 10; return t;
}
static bool pathIs(const DynamicArray<std::string>& path, std::initializer_list<const char*> ids) {
    if (path.size() != ids.size()) return false;
    std::size_t i = 0;
    for (const char* id : ids) if (path[i++] != id) return false;
    return true;
}

int main() {
    // 00001 ~ 00009
    //   1 -> 2 -> 3 -> 4         (긴 경로 1→4: 간선 3개)
    //   1 -> 5 -> 4              (짧은 경로 1→4: 간선 2개)
    //   4 -> 1                   (사이클)
    //   6 -> 7                   (따로 떨어진 부분)
    //   1 -> 1                   (자기 자신으로 반납)
    //   8: 간선 없음,  9: 8 로만 들어감
    DynamicArray<Station> stations;
    for (const char* id : {"00001", "00002", "00003", "00004", "00005",
                           "00006", "00007", "00008", "00009"})
        stations.pushBack(station(id));
    StationTree tree;
    check(buildStationTree(stations, tree), "setup: station tree");

    LinkedList<Trip> trips;
    trips.pushBack(trip("00001", "00002"));
    trips.pushBack(trip("00002", "00003"));
    trips.pushBack(trip("00003", "00004"));
    trips.pushBack(trip("00001", "00005"));
    trips.pushBack(trip("00005", "00004"));
    trips.pushBack(trip("00004", "00001"));
    trips.pushBack(trip("00006", "00007"));
    trips.pushBack(trip("00001", "00001"));
    trips.pushBack(trip("00009", "00008"));
    Graph graph(tree);
    Graph::BuildStats bs;
    graph.buildGraph(trips, bs);

    DynamicArray<std::string> path;
    PathStats stats;

    check(shortestPath(graph, "00001", "00002", path) == PathFound && pathIs(path, {"00001", "00002"}),
          "direct edge 1->2");

    check(shortestPath(graph, "00001", "00004", path, &stats) == PathFound &&
              pathIs(path, {"00001", "00005", "00004"}),
          "picks shorter of two routes 1->5->4 (not 1->2->3->4)");
    check(stats.visitedCount >= 3, "stats: visited count filled");

    check(shortestPath(graph, "00002", "00005", path) == PathFound &&
              pathIs(path, {"00002", "00003", "00004", "00001", "00005"}),
          "multi-hop through cycle 2->3->4->1->5");

    check(shortestPath(graph, "00007", "00006", path) == NoPath && path.empty(),
          "direction matters: 6->7 exists but 7->6 does not");

    check(shortestPath(graph, "00001", "00006", path) == NoPath && path.empty(),
          "disconnected component -> NoPath");

    check(shortestPath(graph, "00008", "00001", path) == NoPath, "station with no out-edges -> NoPath");

    check(shortestPath(graph, "00003", "00003", path, &stats) == PathFound && pathIs(path, {"00003"}) &&
              stats.visitedCount == 1,
          "same start and end -> [start], 0 edges");

    check(shortestPath(graph, "99999", "00001", path) == StationNotFound && path.empty(),
          "unknown start station");
    check(shortestPath(graph, "00001", "99999", path) == StationNotFound, "unknown end station");
    check(shortestPath(graph, "", "00001", path) == StationNotFound, "empty station id");

    // 이전 결과가 남아 있어도 실패 시 path 가 비워지는지
    shortestPath(graph, "00001", "00004", path);
    check(shortestPath(graph, "00001", "00006", path) == NoPath && path.empty(),
          "path cleared on failure after previous success");

    // 사이클 + 자기 자신 간선이 있어도 끝나는지 (위 테스트들이 무한 루프 없이 통과해야 함)
    check(shortestPath(graph, "00001", "00003", path) == PathFound &&
              pathIs(path, {"00001", "00002", "00003"}),
          "self-loop and cycle do not cause infinite loop");

    // 출력
    std::ostringstream out;
    shortestPath(graph, "00001", "00004", path);
    printPath(path, tree, out);
    check(out.str() == "간선 2개 (대여소 3곳)\n00001 S00001\n -> 00005 S00005\n -> 00004 S00004\n",
          "printPath format");

    std::ostringstream empty;
    path.clear();
    printPath(path, tree, empty);
    check(empty.str() == "경로가 없습니다.\n", "printPath on empty path");

    check(std::string(pathResultMessage(NoPath)).size() > 0, "pathResultMessage");

    std::cout << "RESULT: " << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
