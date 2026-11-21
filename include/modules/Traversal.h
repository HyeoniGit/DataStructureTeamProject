#ifndef MODULES_TRAVERSAL_H
#define MODULES_TRAVERSAL_H

#include <cstddef>
#include <ostream>
#include <string>

#include "core/DynamicArray.h"
#include "modules/Graph.h"
#include "modules/StationIndex.h"

// BFS: 대여소 간 최소 간선 경로 (단계 5)
// - 그래프는 방향 그래프 (대여 → 반납). A→B 경로가 있어도 B→A 경로는 없을 수 있다.
// - 간선 가중치(이동 횟수·이용시간)는 보지 않고 "거쳐 가는 간선 수"가 가장 적은 경로를 찾는다.
// - 이웃 목록: Graph::forEachNeighbor
// - 큐: core/Queue.h (방문할 대여소를 넣은 순서대로 꺼냄)
// - 방문 여부 + 바로 이전 대여소: BST<대여소번호, 이전 대여소번호> 하나로 처리
//   (트리에 있으면 방문한 것. 시작 대여소의 이전 값은 "")
//
// 사용 예:
//   DynamicArray<std::string> path;
//   if (shortestPath(graph, "00102", "00345", path) == PathFound)
//       printPath(path, stations, std::cout);
//
// 복잡도: O((V + E) log V) — 각 대여소·간선을 한 번씩 보고, 방문 확인이 BST 검색 O(log V)

enum PathResult {
    PathFound,        // path 에 시작 → 도착 순서로 대여소번호가 들어 있음
    StationNotFound,  // 시작 또는 도착 대여소가 대여소 트리에 없음
    NoPath            // 두 대여소 모두 있지만 이동 기록으로 이어지지 않음
};

// 탐색 통계 (보고서용)
struct PathStats {
    std::size_t visitedCount = 0;  // 큐에 넣은(방문 표시한) 대여소 수
};

// from → to 최소 간선 경로를 찾는다.
// - 성공하면 path 에 [from, ..., to] 를 넣는다 (from == to 이면 [from], 간선 0개)
// - 실패하면 path 는 비어 있다.
// - stats 를 넘기면 탐색 통계를 채운다.
PathResult shortestPath(const Graph& graph, const std::string& from, const std::string& to,
                        DynamicArray<std::string>& path, PathStats* stats = nullptr);

// 경로 출력 (메뉴: BFS 최소 간선 경로)
//   간선 2개 (대여소 3곳)
//   00102 망원역 1번출구 앞
//    -> 00345 합정역 7번출구 앞
//    -> 00501 ...
// 대여소명은 stations 에서 찾아 붙이고, 없으면 번호만 출력한다.
void printPath(const DynamicArray<std::string>& path, const StationTree& stations, std::ostream& out);

// shortestPath 결과를 사람이 읽을 문장으로 (메뉴에서 실패 이유 출력용)
const char* pathResultMessage(PathResult result);

#endif
