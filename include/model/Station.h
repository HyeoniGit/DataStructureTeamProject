#ifndef MODEL_STATION_H
#define MODEL_STATION_H

#include <string>

// 간선: 대여 대여소 → 반납 대여소 (방향 가중 그래프, 2-1)
// - 대여 대여소 노드의 인접 리스트에 들어감
struct Edge {
    std::string toStationId;       // 반납대여소번호
    int         tripCount    = 0;  // 이동 횟수 (가중치 1)
    long long   totalMinutes = 0;  // 이용시간 합 (가중치 2) — 평균 = totalMinutes / tripCount
};

// 대여소 정보 CSV 한 줄 + 그래프 단계에서 쌓는 통계
// - BST 키: id (앞자리 0 때문에 문자열, "00393")
// - 대여소 정보 파일의 정확한 컬럼명·번호 형식은 확인 필요
struct Station {
    // 대여소 정보 CSV (1-1)
    std::string id;             // 대여소번호
    std::string name;           // 대여소명
    std::string address;        // 위치(주소)
    double      latitude  = 0;  // 위도 — 파일에 있는지 확인 필요
    double      longitude = 0;  // 경도 — 파일에 있는지 확인 필요
    int         rackCount = 0;  // 거치대 수

    // 대여이력 처리 중 누적 (2-1)
    int rentCount   = 0;        // 이 대여소에서 대여한 횟수
    int returnCount = 0;        // 이 대여소로 반납한 횟수
    int outDegree   = 0;        // 진출 차수 (서로 다른 반납 대여소 수)
    int inDegree    = 0;        // 진입 차수 (서로 다른 대여 대여소 수)

    // TODO(박민수): 인접 리스트 — core/LinkedList.h 완성 후 추가
    // LinkedList<Edge> edges;
};

#endif
