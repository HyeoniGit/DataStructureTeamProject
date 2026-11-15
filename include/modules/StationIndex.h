#ifndef MODULES_STATION_INDEX_H
#define MODULES_STATION_INDEX_H

#include <ostream>
#include <string>

#include "core/BST.h"
#include "core/DynamicArray.h"
#include "model/Station.h"

// 대여소 번호 트리 (단계 1-1, 3-3)
// - 키: 대여소번호 5자리 문자열 ("00102"), 값: Station
typedef BST<std::string, Station> StationTree;

// 대여소 번호 순으로 정렬된 배열로 대여소 번호 트리를 만든다.
// - 대여소 정보 파일은 번호 오름차순이라 위에서부터 그대로 넣으면 한쪽으로 쏠린다 (높이 = 대여소 수).
//   → 배열의 가운데를 먼저 넣고 왼쪽·오른쪽 절반을 재귀로 넣어 높이를 약 log2(n) 으로 맞춘다.
// - 같은 번호가 여러 번 있으면 처음 것만 넣고 나머지는 건너뛴다.
// - 정렬돼 있지 않으면 아무것도 넣지 않고 false (먼저 병합 정렬로 정렬할 것)
// 복잡도: O(n log n)
bool buildStationTree(const DynamicArray<Station>& sortedStations, StationTree& tree);

// stations 가 대여소번호 오름차순(같은 번호 허용)인지
bool isSortedById(const DynamicArray<Station>& stations);

// 대여소 정보 한 건 출력 (메뉴: 대여소 검색)
void printStation(const Station& station, std::ostream& out);

#endif
