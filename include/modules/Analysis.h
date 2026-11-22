#ifndef MODULES_ANALYSIS_H
#define MODULES_ANALYSIS_H

#include <cstddef>
#include <ostream>
#include <string>

#include "core/DynamicArray.h"
#include "core/LinkedList.h"
#include "model/Trip.h"
#include "modules/StationIndex.h"

// 정렬 · TOP-N · 이분 탐색 (단계 2-2, 3-1, 3-2 / 발표 3-1 ~ 3-3)
// - 대여 기록: 7개 기준 정렬, TOP-N, 이분 탐색
// - 대여소: 이용 횟수·차수 기준 정렬, TOP-N (파일 아래쪽 "대여소 전용")
//
// 흐름 (대여 기록)
//   대여이력 연결 리스트 → 기록 주소를 포인터 배열에 복사 (buildTripPtrArray)
//   → 기준을 골라 병합 정렬 (sortTrips) → 앞에서 N개 (topN) / 이분 탐색 (findTrips...)
//
// - 연결 리스트는 가운데로 바로 갈 수 없어 정렬·이분 탐색을 못 하므로 포인터 배열로 옮긴다.
// - 배열에는 기록 주소만 담는다. 정렬해도 원본 기록은 그대로이고, 주소로 기록 전체를 읽는다.
// - 정렬 결과는 보관하지 않는다. 기준이 바뀔 때마다 같은 배열을 다시 정렬한다.
//   (포인터 배열 1개 ≈ 418만 × 8B ≈ 33MB, 기준 7개를 다 보관하면 약 230MB)
// - 대여이력 원본은 반납일시 순이라 대여일시는 뒤섞여 있다 → 대여일시로 찾으려면 정렬이 필요하다.

// ===================== 템플릿 알고리즘 =====================

namespace analysis_detail {

// arr[lo, mid) 와 arr[mid, hi) 는 각각 정렬된 상태 → temp 에 합친 뒤 arr 로 되돌린다.
// 같은 값이면 왼쪽(원래 앞에 있던 것)을 먼저 넣어 순서를 유지한다 (안정 정렬).
template <typename T, typename Less>
void merge(DynamicArray<T>& arr, DynamicArray<T>& temp,
           std::size_t lo, std::size_t mid, std::size_t hi, Less& less) {
    std::size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        if (less(arr[j], arr[i])) temp[k++] = arr[j++];   // 오른쪽이 확실히 작을 때만 오른쪽
        else                      temp[k++] = arr[i++];
    }
    while (i < mid) temp[k++] = arr[i++];
    while (j < hi)  temp[k++] = arr[j++];
    for (k = lo; k < hi; ++k) arr[k] = temp[k];
}

// arr[lo, hi) 를 반으로 나눠 각각 정렬한 뒤 합친다.
// 재귀 깊이 ≈ log2(n) (418만 건이어도 약 22단계)
template <typename T, typename Less>
void mergeSortRange(DynamicArray<T>& arr, DynamicArray<T>& temp,
                    std::size_t lo, std::size_t hi, Less& less) {
    if (hi - lo < 2) return;
    std::size_t mid = lo + (hi - lo) / 2;
    mergeSortRange(arr, temp, lo, mid, less);
    mergeSortRange(arr, temp, mid, hi, less);
    merge(arr, temp, lo, mid, hi, less);
}

}  // namespace analysis_detail

// 병합 정렬 (안정 정렬)
// - less(a, b): a 가 b 보다 앞에 와야 하면 true. 내림차순은 less 의 방향만 바꿔 넘긴다.
// - 임시 배열은 한 번만 만들어 재귀 전체에서 같이 쓴다.
// 복잡도: 항상 O(n log n), 추가 공간 O(n)
template <typename T, typename Less>
void mergeSort(DynamicArray<T>& arr, Less less) {
    if (arr.size() < 2) return;
    DynamicArray<T> temp(arr.size(), arr[0]);
    analysis_detail::mergeSortRange(arr, temp, 0, arr.size(), less);
}

// 이분 탐색 (발표 p.15 의사 코드)
// - arr 는 getKey 기준 오름차순으로 정렬돼 있어야 한다.
// - getKey(원소) → 비교할 값
// - 찾으면 true 와 그 위치(foundIndex), 못 찾으면 false
//   (같은 값이 여러 개면 그중 아무 하나의 위치. 전부 필요하면 lowerBound / upperBound)
// 복잡도: O(log n)
template <typename T, typename K, typename GetKey>
bool binarySearch(const DynamicArray<T>& arr, const K& key, GetKey getKey,
                  std::size_t& foundIndex) {
    if (arr.empty()) return false;
    std::size_t lo = 0, hi = arr.size() - 1;
    while (lo <= hi) {
        std::size_t mid = lo + (hi - lo) / 2;
        const K& midKey = getKey(arr[mid]);
        if (midKey == key) {
            foundIndex = mid;
            return true;
        } else if (midKey < key) {
            lo = mid + 1;
        } else {
            if (mid == 0) break;   // size_t 라 0 - 1 이 되지 않도록
            hi = mid - 1;
        }
    }
    return false;
}

// key 이상인 첫 위치 (모두 key 보다 작으면 arr.size())
// 복잡도: O(log n)
template <typename T, typename K, typename GetKey>
std::size_t lowerBound(const DynamicArray<T>& arr, const K& key, GetKey getKey) {
    std::size_t lo = 0, hi = arr.size();   // 답은 [lo, hi] 안에 있다
    while (lo < hi) {
        std::size_t mid = lo + (hi - lo) / 2;
        if (getKey(arr[mid]) < key) lo = mid + 1;
        else                        hi = mid;
    }
    return lo;
}

// key 보다 큰 첫 위치 (모두 key 이하이면 arr.size())
// 복잡도: O(log n)
template <typename T, typename K, typename GetKey>
std::size_t upperBound(const DynamicArray<T>& arr, const K& key, GetKey getKey) {
    std::size_t lo = 0, hi = arr.size();
    while (lo < hi) {
        std::size_t mid = lo + (hi - lo) / 2;
        if (key < getKey(arr[mid])) hi = mid;
        else                        lo = mid + 1;
    }
    return lo;
}

// ===================== 대여이력 전용 =====================

// 정렬 기준: Trip 의 7개 필드
enum class TripKey {
    BikeId,          // 자전거번호      (문자열, "SPB-" + 5자리 → 사전순 = 번호순)
    RentTime,        // 대여일시        (20260601000133)
    RentStation,     // 대여 대여소번호 (5자리 문자열 → 사전순 = 번호순)
    ReturnTime,      // 반납일시
    ReturnStation,   // 반납대여소번호
    Minutes,         // 이용시간(분)
    Distance         // 이용거리(M)
};

const int TRIP_KEY_COUNT = 7;

// 기준 이름 ("대여일시" 등) — 메뉴·출력용
const char* tripKeyName(TripKey key);

// 문자열 기준(BikeId, RentStation, ReturnStation)이면 true, 숫자 기준이면 false
bool isStringKey(TripKey key);

// 대여이력 연결 리스트의 기록 주소를 순서대로 out 에 담는다 (기존 내용은 지움).
// - trips 는 out 을 쓰는 동안 살아 있어야 한다 (주소를 가리키므로)
// 복잡도: O(n)
void buildTripPtrArray(const LinkedList<Trip>& trips, DynamicArray<const Trip*>& out);

// key 기준 병합 정렬. descending 이 true 면 큰 값부터.
// 같은 값끼리는 정렬 전 순서를 유지한다.
// 복잡도: O(n log n)
void sortTrips(DynamicArray<const Trip*>& arr, TripKey key, bool descending);

// TOP-N: key 기준 내림차순으로 정렬한 뒤, 앞에서 꺼낼 개수 min(n, 전체)를 돌려준다.
// → arr[0] ~ arr[반환값 - 1] 이 상위 N개 (발표 3-2)
// 복잡도: 정렬 O(n log n) + 꺼내기 O(N)
std::size_t topN(DynamicArray<const Trip*>& arr, TripKey key, std::size_t n);

// ----- 이분 탐색 (arr 는 sortTrips(arr, key, false) 로 오름차순 정렬돼 있어야 한다) -----

// 숫자 기준(RentTime, ReturnTime, Minutes, Distance)에서 from 이상 to 이하인 기록의 범위
// → arr[begin] ~ arr[end - 1]. 하나도 없으면 false (begin == end)
// - 문자열 기준을 넣으면 검색하지 않고 false
// 예: 대여일시 6월 1일 08시대 → from 20260601080000, to 20260601085959
// 복잡도: O(log n)
bool findTripsInRange(const DynamicArray<const Trip*>& arr, TripKey key,
                      double from, double to, std::size_t& begin, std::size_t& end);

// 문자열 기준(BikeId, RentStation, ReturnStation)에서 id 와 같은 기록 전부의 범위
// - 숫자 기준을 넣으면 검색하지 않고 false
// 예: 대여소 "00393" 에서 빌린 기록 전부 → 정렬 기준 RentStation
// 복잡도: O(log n)
bool findTripsById(const DynamicArray<const Trip*>& arr, TripKey key,
                   const std::string& id, std::size_t& begin, std::size_t& end);

// 정확히 일치하는 기록 하나 찾기 (발표 p.15 의사 코드 그대로) — 성공 여부 + 그 기록 주소
// - 못 찾거나 기준 종류가 맞지 않으면 nullptr
// 복잡도: O(log n)
const Trip* findTripExact(const DynamicArray<const Trip*>& arr, TripKey key, double value);             // 숫자 기준
const Trip* findTripExact(const DynamicArray<const Trip*>& arr, TripKey key, const std::string& id);   // 문자열 기준

// arr[begin, end) 의 기록을 한 줄씩 출력한다. 대여소 이름은 대여소 번호 트리에서 찾는다.
// 예: "1. SPB-58380  06-01 00:01 서울체육고등학교 앞 -> 06-01 00:01 서울체육고등학교 앞  (0분, 0m)"
void printTrips(const DynamicArray<const Trip*>& arr, std::size_t begin, std::size_t end,
                const StationTree& stations, std::ostream& out);

// ===================== 대여소 전용 =====================
//
// 흐름
//   대여소 번호 트리 → 대여소 노드 주소를 포인터 배열에 복사 (buildStationPtrArray)
//   → 이용 횟수 등 기준으로 병합 정렬 (sortStations) → 앞에서 N개 (topNStations)
//
// - 횟수·차수는 그래프 구축(Graph::buildGraph) 때 Station 에 쌓인다.
//   → 그래프를 만든 뒤에 배열을 만들어야 값이 0 이 아니다.
// - 대여소는 트리 노드 안에 있고 노드는 재할당되지 않으므로 주소를 그대로 담아도 된다.
// - 트리 중위 순회로 담으므로 정렬 전 배열은 대여소번호 순
// - 같은 값이면 대여소번호가 작은 대여소가 앞 (직전 정렬 기준과 상관없이 결과가 같음)

// 정렬 기준: 그래프 단계에서 쌓은 대여소 통계
enum class StationKey {
    RentCount,     // 이 대여소에서 대여한 횟수
    ReturnCount,   // 이 대여소로 반납한 횟수
    TotalCount,    // 대여 + 반납 (대여소 이용량)
    OutDegree,     // 진출 차수 (서로 다른 반납 대여소 수)
    InDegree       // 진입 차수 (서로 다른 대여 대여소 수)
};

const int STATION_KEY_COUNT = 5;

// 기준 이름 ("대여 횟수" 등) — 메뉴·출력용
const char* stationKeyName(StationKey key);

// 대여소 번호 트리의 대여소 주소를 번호 순으로 out 에 담는다 (기존 내용은 지움).
// - tree 는 out 을 쓰는 동안 살아 있어야 한다
// 복잡도: O(V)
void buildStationPtrArray(const StationTree& tree, DynamicArray<const Station*>& out);

// key 기준 병합 정렬. descending 이 true 면 큰 값부터. 같은 값이면 대여소번호 오름차순.
// 복잡도: O(V log V)
void sortStations(DynamicArray<const Station*>& arr, StationKey key, bool descending);

// 대여소 TOP-N: key 기준 내림차순 정렬 후 꺼낼 개수 min(n, 전체)를 돌려준다.
// → arr[0] ~ arr[반환값 - 1] 이 상위 N개
// 복잡도: 정렬 O(V log V) + 꺼내기 O(N)
std::size_t topNStations(DynamicArray<const Station*>& arr, StationKey key, std::size_t n);

// arr[begin, end) 의 대여소를 한 줄씩 출력한다.
// 예: "1. 00102 망원역 1번출구 앞  대여 1234 / 반납 1100 / 진출 85 / 진입 90"
void printStations(const DynamicArray<const Station*>& arr, std::size_t begin, std::size_t end,
                   std::ostream& out);

#endif
