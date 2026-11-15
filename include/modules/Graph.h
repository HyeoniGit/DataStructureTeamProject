#ifndef DATASTRUCTURE_GRAPH_H
#define DATASTRUCTURE_GRAPH_H

#include "modules/StationIndex.h"
#include "model/Trip.h"
#include "core/LinkedList.h"
#include <string>

/*
 * Graph: 대여소 간 이동 기록을 방향 가중 그래프로 누적하는 인터페이스.
 *
 * 소유권: Graph는 StationTree를 참조만 한다. StationTree를 먼저 만들고
 * Graph보다 오래 유지해야 한다. Trip 목록도 Graph가 소유하지 않는다.
 * 저장 위치: 그래프 간선은 Graph 객체가 아닌 각 Station::edges(LinkedList<Edge>)에
 * 보관한다. Edge::toStationId는 도착 대여소의 5자리 문자열 ID이다.
 *
 * 사용 순서:
 *   StationTree stations; // 대여소를 미리 삽입하거나 buildStationTree로 구축
 *   Graph graph(stations);
 *   Graph::BuildStats stats;
 *   graph.buildGraph(trips, stats);
 *   const Edge* e = graph.getEdge("00001", "00002");
 *
 * 주의: 같은 StationTree에 다른 Graph를 만들어 다시 buildGraph 하면 통계가 중복된다.
 * 또한 구축 이후 Trip을 추가해도 자동으로 반영되지 않는다.
 */
class Graph
{
public:
    // 개별 Trip을 내부에서 처리한 결과. buildGraph의 통계 항목에 대응한다.
    enum Result
    {
        Added,
        Updated,
        MissingStation,
        InvalidTrip
    };

    // 한 Graph 인스턴스는 buildGraph를 최초 1회만 수행한다.
    enum BuildResult
    {
        BuildSuccess,
        AlreadyBuilt
    };

    // 처리한 Trip 건수의 분류: 네 항목의 합은 입력 Trip 개수와 같다.
    struct BuildStats
    {
        long long added;
        long long updated;
        long long missingStation;
        long long invalidTrip;

        BuildStats()
            : added(0), updated(0),
              missingStation(0), invalidTrip(0)
        {
        }
    };

private:
    StationTree& stations_; // 소유하지 않는 BST 참조

    bool built_;
    BuildStats stats_;

    // 내부용: from의 인접 리스트에서 도착 ID를 선형 탐색한다. O(outDegree).
    Edge* findEdge(Station* from, const std::string& toId)
    {
        if (from == nullptr)
            return nullptr;

        auto* node = from->edges.head();
        while (node != nullptr)
        {
            if (node->data.toStationId == toId)
                return &(node->data);
            node = node->next;
        }
        return nullptr;
    }

    // 내부용: ID/시간 유효성 검사 -> BST 검색 -> 기존 간선 갱신 또는 생성.
    // 존재하지 않는 대여소가 있으면 통계/간선을 수정하지 않는다.
    Result recordTrip(const Trip& trip)
    {
        if (trip.rentStationId.empty() ||
            trip.returnStationId.empty() ||
            trip.useMinutes < 0)
            return InvalidTrip;

        Station* from = stations_.find(trip.rentStationId);
        Station* to = stations_.find(trip.returnStationId);
        if (from == nullptr || to == nullptr)
            return MissingStation;

        Edge* edge = findEdge(from, trip.returnStationId);
        if (edge != nullptr)
        {
            edge->tripCount += 1;
            edge->totalMinutes += trip.useMinutes;
            from->rentCount += 1;
            to->returnCount += 1;
            return Updated;
        }

        Edge newEdge;
        newEdge.toStationId = trip.returnStationId;
        newEdge.tripCount = 1;
        newEdge.totalMinutes = trip.useMinutes;
        from->edges.pushBack(newEdge);

        from->outDegree += 1;
        to->inDegree += 1;
        from->rentCount += 1;
        to->returnCount += 1;
        return Added;
    }

public:
    // 생성만으로는 간선을 구축하지 않는다. buildGraph를 별도로 호출한다.
    explicit Graph(StationTree& stations)
        : stations_(stations), built_(false), stats_()
    {
    }

    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;

    // Trip 연결 리스트를 순서대로 읽고, 처리 결과를 output에 복사한다.
    // 첫 호출: BuildSuccess, 이후 호출: AlreadyBuilt (기존 통계만 반환).
    // 시간: Trip 수 T, BST 검색 비용, 출발 대여소 인접 리스트 탐색 비용의 합.
    // 초기화/재구축 기능은 제공하지 않는다.
    BuildResult buildGraph(LinkedList<Trip>& trips, BuildStats& output)
    {
        if (built_)
        {
            output = stats_;
            return AlreadyBuilt;
        }

        auto* node = trips.head();
        while (node != nullptr)
        {
            Result result = recordTrip(node->data);
            if (result == Added)
                stats_.added += 1;
            else if (result == Updated)
                stats_.updated += 1;
            else if (result == MissingStation)
                stats_.missingStation += 1;
            else
                stats_.invalidTrip += 1;
            node = node->next;
        }

        built_ = true;
        output = stats_;
        return BuildSuccess;
    }

    // 현재 Graph 객체에서 buildGraph가 최초 실행되었는지 반환한다.
    bool isBuilt() const
    {
        return built_;
    }

    // 현재까지 저장한 처리 통계의 복사본을 반환한다.
    BuildStats buildStats() const
    {
        return stats_;
    }

    // 간선 유무가 아닌 BST 내 대여소 존재 여부를 검사한다.
    bool containsStation(const std::string& id) const
    {
        return stations_.find(id) != nullptr;
    }

    // BFS용: stationId의 출발 간선들을 순회하며 visit(도착ID)를 호출한다.
    // 해당 대여소가 없거나 간선이 없으면 콜백을 호출하지 않는다.
    // 방문 여부 확인과 Queue 삽입은 BFS 담당 코드에서 수행한다.
    // 콜백 인자는 내부 문자열의 const 참조이므로 오래 보관하지 않는다.
    template <typename Visit>
    void forEachNeighbor(const std::string& stationId, Visit visit) const
    {
        Station* station = stations_.find(stationId);
        if (station == nullptr)
            return;

        auto* node = station->edges.head();
        while (node != nullptr)
        {
            visit(node->data.toStationId);
            node = node->next;
        }
    }

    // 공개 조회: fromId -> toId 간선을 찾는다. 없으면 nullptr.
    // 반환 포인터는 StationTree/간선이 살아 있는 동안만 사용한다.
    // tripCount(이동 횟수), totalMinutes(총 분)를 읽을 수 있다.
    const Edge* getEdge(const std::string& fromId,
                        const std::string& toId) const
    {
        Station* from = stations_.find(fromId);
        if (from == nullptr)
            return nullptr;

        const auto* node = from->edges.head();
        while (node != nullptr)
        {
            if (node->data.toStationId == toId)
                return &(node->data);
            node = node->next;
        }
        return nullptr;
    }

 

    // from -> to 간선은 존재하고 역방향 간선은 없는 경우에만 true.
    // 자기 자신으로의 이동이나 존재하지 않는 대여소는 false.
    // 이동 횟수의 비대칭이 아니라 역방향 간선의 부재를 판별한다.
    bool isOneWay(const std::string& fromId,
                  const std::string& toId) const
    {
        if (fromId == toId)
            return false;
        if (!containsStation(fromId) || !containsStation(toId))
            return false;
        return getEdge(fromId, toId) != nullptr &&
               getEdge(toId, fromId) == nullptr;
    }

    // 평균 이용시간(분) = totalMinutes / tripCount; 0으로 나누지 않는다.
    // nullptr 또는 0건이면 0.0을 반환하므로 호출자는 간선 유무를 따로 확인한다.
    static double averageMinutes(const Edge* edge)
    {
        if (edge == nullptr || edge->tripCount <= 0)
            return 0.0;
        return (double)edge->totalMinutes / (double)edge->tripCount;
    }
};

#endif
