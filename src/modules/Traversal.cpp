#include "modules/Traversal.h"

#include "core/BST.h"
#include "core/Queue.h"

namespace {

// 도착 대여소에서 prev 를 거꾸로 따라가 시작 대여소까지 모은 뒤 뒤집는다.
void buildPath(const BST<std::string, std::string>& prev, const std::string& to,
               DynamicArray<std::string>& path) {
    std::string cur = to;
    while (!cur.empty()) {
        path.pushBack(cur);
        cur = *prev.find(cur);  // 경로 위의 대여소는 모두 방문했으므로 항상 있음
    }
    for (std::size_t i = 0, j = path.size() - 1; i < j; ++i, --j) {
        std::string tmp = path[i];
        path[i] = path[j];
        path[j] = tmp;
    }
}

}  // namespace

PathResult shortestPath(const Graph& graph, const std::string& from, const std::string& to,
                        DynamicArray<std::string>& path, PathStats* stats) {
    path.clear();
    if (stats != nullptr) *stats = PathStats();

    if (!graph.containsStation(from) || !graph.containsStation(to)) return StationNotFound;

    if (from == to) {
        path.pushBack(from);
        if (stats != nullptr) stats->visitedCount = 1;
        return PathFound;
    }

    BST<std::string, std::string> prev;  // 방문한 대여소 → 바로 이전 대여소
    Queue<std::string> queue;
    prev.insert(from, "");
    queue.enqueue(from);

    bool found = false;
    while (!queue.empty() && !found) {
        std::string cur = queue.front();  // 복사: dequeue 하면 노드가 지워짐
        queue.dequeue();

        graph.forEachNeighbor(cur, [&](const std::string& next) {
            if (found || prev.contains(next)) return;  // 이미 찾았거나 방문한 대여소
            prev.insert(next, cur);                    // 콜백 인자는 복사해서 저장
            queue.enqueue(next);
            if (next == to) found = true;              // 처음 발견한 순간이 최소 간선
        });
    }

    if (stats != nullptr) stats->visitedCount = prev.size();
    if (!found) return NoPath;

    buildPath(prev, to, path);
    return PathFound;
}

void printPath(const DynamicArray<std::string>& path, const StationTree& stations, std::ostream& out) {
    if (path.empty()) {
        out << "경로가 없습니다.\n";
        return;
    }
    out << "간선 " << path.size() - 1 << "개 (대여소 " << path.size() << "곳)\n";
    for (std::size_t i = 0; i < path.size(); ++i) {
        out << (i == 0 ? "" : " -> ") << path[i];
        const Station* s = stations.find(path[i]);
        if (s != nullptr && !s->name.empty()) out << ' ' << s->name;
        out << '\n';
    }
}

const char* pathResultMessage(PathResult result) {
    switch (result) {
        case PathFound:       return "경로를 찾았습니다.";
        case StationNotFound: return "없는 대여소 번호입니다.";
        case NoPath:          return "두 대여소를 잇는 이동 기록이 없습니다.";
    }
    return "";
}
