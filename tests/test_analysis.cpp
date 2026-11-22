// 정렬 · TOP-N · 이분 탐색 테스트
// 실행: make test
#include <cstdio>
#include <sstream>
#include <string>

#include "core/DynamicArray.h"
#include "core/LinkedList.h"
#include "modules/Analysis.h"
#include "modules/CsvParser.h"
#include "modules/Graph.h"
#include "modules/StationIndex.h"

namespace {

int failures = 0;

void check(bool ok, const char* what) {
    std::printf("[%s] %s\n", ok ? " OK " : "FAIL", what);
    if (!ok) ++failures;
}

const TripKey ALL_KEYS[TRIP_KEY_COUNT] = {
    TripKey::BikeId, TripKey::RentTime, TripKey::RentStation, TripKey::ReturnTime,
    TripKey::ReturnStation, TripKey::Minutes, TripKey::Distance};

Trip makeTrip(const std::string& bike, long long rent, const std::string& from,
              long long ret, const std::string& to, int minutes, double meters) {
    Trip t;
    t.bikeId = bike;
    t.rentTime = rent;
    t.rentStationId = from;
    t.returnTime = ret;
    t.returnStationId = to;
    t.useMinutes = minutes;
    t.distanceM = meters;
    return t;
}

// 원본처럼 반납일시 순, 대여일시는 뒤섞임. 값이 같은 기록을 일부러 섞어 둠
void fillTrips(LinkedList<Trip>& trips) {
    trips.pushBack(makeTrip("SPB-30000", 20260601080500, "00393", 20260601081000, "00102", 5, 900.0));
    trips.pushBack(makeTrip("SPB-10000", 20260601080000, "00102", 20260601082000, "00393", 20, 3000.0));
    trips.pushBack(makeTrip("SPB-20000", 20260601081500, "00393", 20260601083000, "00207", 15, 2500.0));
    trips.pushBack(makeTrip("SPB-10000", 20260601083000, "00393", 20260601084000, "00102", 10, 1500.0));
    trips.pushBack(makeTrip("SPB-50000", 20260601075000, "00207", 20260601085000, "00393", 60, 9000.0));
    trips.pushBack(makeTrip("SPB-40000", 20260601084500, "00102", 20260601090000, "00207", 15, 2500.0));
}

// 원래 몇 번째 기록이었는지 (안정성 확인용)
std::size_t originalIndex(const DynamicArray<const Trip*>& original, const Trip* t) {
    for (std::size_t i = 0; i < original.size(); ++i)
        if (original[i] == t) return i;
    return original.size();
}

// key 기준으로 정렬돼 있고, 같은 값끼리는 원래 순서를 지키는지
bool sortedAndStable(const DynamicArray<const Trip*>& arr, const DynamicArray<const Trip*>& original,
                     TripKey key, bool descending) {
    DynamicArray<const Trip*> pair;
    for (std::size_t i = 1; i < arr.size(); ++i) {
        // 앞뒤 두 개를 오름차순으로 다시 정렬해 보고, 순서가 바뀌는지로 비교
        pair.clear();
        pair.pushBack(arr[i - 1]);
        pair.pushBack(arr[i]);
        sortTrips(pair, key, descending);
        if (pair[0] != arr[i - 1]) return false;   // 정렬 순서 위반

        // 같은 값인지: 반대 방향으로 정렬해도 그대로면 같은 값
        pair.clear();
        pair.pushBack(arr[i - 1]);
        pair.pushBack(arr[i]);
        sortTrips(pair, key, !descending);
        bool equal = (pair[0] == arr[i - 1]);
        if (equal && originalIndex(original, arr[i - 1]) > originalIndex(original, arr[i]))
            return false;                          // 같은 값인데 원래 순서가 뒤집힘
    }
    return true;
}

void testMergeSortBasic() {
    DynamicArray<int> empty;
    mergeSort(empty, [](int a, int b) { return a < b; });
    check(empty.empty(), "빈 배열 정렬: 그대로");

    DynamicArray<int> one;
    one.pushBack(7);
    mergeSort(one, [](int a, int b) { return a < b; });
    check(one.size() == 1 && one[0] == 7, "원소 1개 정렬: 그대로");

    int values[] = {8, 2, 4, 3, 7, 1, 5};   // 발표 p.12 예시
    DynamicArray<int> arr;
    for (int v : values) arr.pushBack(v);
    mergeSort(arr, [](int a, int b) { return a < b; });
    std::string s;
    for (std::size_t i = 0; i < arr.size(); ++i) s += std::to_string(arr[i]);
    check(s == "1234578", "발표 예시 8 2 4 3 7 1 5 → 1 2 3 4 5 7 8");

    mergeSort(arr, [](int a, int b) { return a > b; });
    s.clear();
    for (std::size_t i = 0; i < arr.size(); ++i) s += std::to_string(arr[i]);
    check(s == "8754321", "비교 방향만 바꿔 내림차순");

    // 안정성: (값, 원래 순서) — 값만으로 정렬
    struct Item { int value; int order; };
    DynamicArray<Item> items;
    int vs[] = {3, 1, 3, 2, 1, 3};
    for (int i = 0; i < 6; ++i) items.pushBack(Item{vs[i], i});
    mergeSort(items, [](const Item& a, const Item& b) { return a.value < b.value; });
    s.clear();
    for (std::size_t i = 0; i < items.size(); ++i) s += std::to_string(items[i].order);
    check(s == "143025", "안정 정렬: 같은 값은 원래 순서 유지");
}

void testBinarySearchBasic() {
    DynamicArray<int> arr;
    int vs[] = {1, 3, 5, 7, 9, 11, 13, 15};   // 발표 p.5 예시
    for (int v : vs) arr.pushBack(v);
    auto self = [](const int& v) -> const int& { return v; };

    std::size_t idx = 99;
    check(binarySearch(arr, 15, self, idx) && idx == 7, "이분 탐색: 15 → 위치 7 (발표 예시)");
    check(binarySearch(arr, 1, self, idx) && idx == 0, "이분 탐색: 첫 원소");
    check(binarySearch(arr, 7, self, idx) && idx == 3, "이분 탐색: 가운데");
    check(!binarySearch(arr, 0, self, idx), "이분 탐색: 최솟값보다 작은 값 → 실패");
    check(!binarySearch(arr, 8, self, idx), "이분 탐색: 사이 값 → 실패");
    check(!binarySearch(arr, 99, self, idx), "이분 탐색: 최댓값보다 큰 값 → 실패");

    DynamicArray<int> empty;
    check(!binarySearch(empty, 1, self, idx), "이분 탐색: 빈 배열 → 실패");

    DynamicArray<int> dup;
    int ds[] = {1, 2, 2, 2, 5};
    for (int v : ds) dup.pushBack(v);
    check(lowerBound(dup, 2, self) == 1 && upperBound(dup, 2, self) == 4, "lower/upper bound: 2 는 [1, 4)");
    check(lowerBound(dup, 3, self) == 4 && upperBound(dup, 3, self) == 4, "lower/upper bound: 없는 3 → 빈 범위");
    check(lowerBound(dup, 0, self) == 0 && upperBound(dup, 9, self) == 5, "lower/upper bound: 양 끝");
}

void testTripSort() {
    LinkedList<Trip> trips;
    fillTrips(trips);

    DynamicArray<const Trip*> original;
    buildTripPtrArray(trips, original);
    check(original.size() == 6 && original[0] == &trips.head()->data, "포인터 배열: 개수와 순서 = 연결 리스트");

    bool allOk = true;
    for (TripKey key : ALL_KEYS) {
        for (int d = 0; d < 2; ++d) {
            DynamicArray<const Trip*> arr = original;
            sortTrips(arr, key, d == 1);
            if (!sortedAndStable(arr, original, key, d == 1)) {
                allOk = false;
                std::printf("       → %s %s 실패\n", tripKeyName(key), d ? "내림차순" : "오름차순");
            }
        }
    }
    check(allOk, "7개 기준 × 오름/내림차순: 정렬 + 안정성");

    DynamicArray<const Trip*> arr = original;
    sortTrips(arr, TripKey::RentTime, false);
    check(arr[0]->rentTime == 20260601075000 && arr[5]->rentTime == 20260601084500,
          "대여일시 오름차순: 07:50 ~ 08:45");
    check(original[0]->rentTime == 20260601080500, "정렬은 배열만 바꾸고 원본 리스트는 그대로");
}

void testTopN() {
    LinkedList<Trip> trips;
    fillTrips(trips);
    DynamicArray<const Trip*> arr;
    buildTripPtrArray(trips, arr);

    std::size_t n = topN(arr, TripKey::Minutes, 3);
    check(n == 3 && arr[0]->useMinutes == 60 && arr[1]->useMinutes == 20 && arr[2]->useMinutes == 15,
          "TOP-3 이용시간: 60, 20, 15");
    check(arr[2]->bikeId == "SPB-20000", "TOP-N 동점(15분)은 원래 순서가 앞선 기록");

    check(topN(arr, TripKey::Distance, 0) == 0, "TOP-0 → 0개");
    check(topN(arr, TripKey::Distance, 100) == 6, "N 이 전체보다 크면 전체 개수");
}

void testTripSearch() {
    LinkedList<Trip> trips;
    fillTrips(trips);
    DynamicArray<const Trip*> arr;
    buildTripPtrArray(trips, arr);

    std::size_t b = 0, e = 0;
    sortTrips(arr, TripKey::RentTime, false);
    check(findTripsInRange(arr, TripKey::RentTime, 20260601080000, 20260601081500, b, e) && e - b == 3,
          "대여일시 08:00~08:15: 경계 포함 3건");
    check(!findTripsInRange(arr, TripKey::RentTime, 20260601090000, 20260601100000, b, e) && b == e,
          "범위 밖 기간 → 없음");
    check(!findTripsInRange(arr, TripKey::RentTime, 20260601083000, 20260601080000, b, e) && b == e,
          "시작 > 끝 → 없음");

    sortTrips(arr, TripKey::Minutes, false);
    check(findTripsInRange(arr, TripKey::Minutes, 15, 60, b, e) && e - b == 4, "이용시간 15~60분: 4건 (15, 15, 20, 60)");

    sortTrips(arr, TripKey::RentStation, false);
    check(findTripsById(arr, TripKey::RentStation, "00393", b, e) && e - b == 3,
          "대여 대여소 00393 에서 빌린 기록 3건");
    check(!findTripsById(arr, TripKey::RentStation, "99999", b, e), "없는 대여소 → 없음");

    sortTrips(arr, TripKey::BikeId, false);
    check(findTripsById(arr, TripKey::BikeId, "SPB-10000", b, e) && e - b == 2, "자전거 SPB-10000 기록 2건");

    sortTrips(arr, TripKey::RentTime, false);
    const Trip* t = findTripExact(arr, TripKey::RentTime, 20260601081500.0);
    check(t != nullptr && t->bikeId == "SPB-20000", "정확히 일치: 대여일시 08:15 → SPB-20000 주소");
    check(findTripExact(arr, TripKey::RentTime, 20260601081501.0) == nullptr, "정확히 일치: 없는 일시 → nullptr");

    sortTrips(arr, TripKey::BikeId, false);
    t = findTripExact(arr, TripKey::BikeId, std::string("SPB-50000"));
    check(t != nullptr && t->useMinutes == 60, "정확히 일치: 자전거 SPB-50000 → 60분 기록 주소");

    // 기준 종류가 맞지 않으면 검색하지 않음
    check(!findTripsInRange(arr, TripKey::BikeId, 0, 1e18, b, e) && b == e,
          "숫자 범위 검색에 문자열 기준 → false");
    check(!findTripsById(arr, TripKey::Minutes, "15", b, e) && b == e,
          "번호 검색에 숫자 기준 → false");
    check(findTripExact(arr, TripKey::BikeId, 15.0) == nullptr &&
          findTripExact(arr, TripKey::Minutes, std::string("15")) == nullptr,
          "정확히 일치 검색에 기준 종류가 다름 → nullptr");
}

void testPrint() {
    LinkedList<Trip> trips;
    fillTrips(trips);
    DynamicArray<const Trip*> arr;
    buildTripPtrArray(trips, arr);

    StationTree stations;
    Station s;
    s.id = "00393";
    s.name = "동대문역 8번 출구";
    stations.insert(s.id, s);

    std::ostringstream out;
    printTrips(arr, 0, 1, stations, out);
    check(out.str() == "1. SPB-30000  06-01 08:05 동대문역 8번 출구 -> 06-01 08:10 00102  (5분, 900m)\n",
          "출력: 대여소 이름은 트리에서, 없으면 번호");
}

void testSampleCsv() {
    LinkedList<Trip> trips;
    ParseResult result;
    if (!loadTrips("data/sample.csv", trips, result)) {
        check(false, "data/sample.csv 열기 (저장소 최상위에서 실행)");
        return;
    }
    DynamicArray<const Trip*> arr;
    buildTripPtrArray(trips, arr);

    bool rentSortedBefore = true;
    for (std::size_t i = 1; i < arr.size(); ++i)
        if (arr[i]->rentTime < arr[i - 1]->rentTime) rentSortedBefore = false;
    check(!rentSortedBefore, "sample.csv: 원본은 대여일시 순이 아님 (정렬 필요)");

    sortTrips(arr, TripKey::RentTime, false);
    bool sortedAfter = true;
    for (std::size_t i = 1; i < arr.size(); ++i)
        if (arr[i]->rentTime < arr[i - 1]->rentTime) sortedAfter = false;
    check(sortedAfter && arr.size() == trips.size(), "sample.csv: 대여일시 정렬 후 오름차순");
}


// ----- 대여소 정렬 · TOP-N -----

void addStation(StationTree& tree, const std::string& id, const std::string& name) {
    Station s;
    s.id = id;
    s.name = name;
    tree.insert(id, s);
}

// fillTrips 의 6건으로 그래프를 쌓으면
//   대여: 00393 3, 00102 2, 00207 1, 00500 0
//   반납: 00102 2, 00207 2, 00393 2, 00500 0
//   진출: 00102 2, 00393 2, 00207 1 / 진입: 00207 2, 00393 2, 00102 1
std::string ids(const DynamicArray<const Station*>& arr, std::size_t n) {
    std::string r;
    for (std::size_t i = 0; i < n && i < arr.size(); ++i) {
        if (i > 0) r += ",";
        r += arr[i]->id;
    }
    return r;
}

void testStationSort() {
    StationTree stations;
    addStation(stations, "00393", "동대문역 8번 출구");
    addStation(stations, "00102", "망원역 1번출구 앞");
    addStation(stations, "00500", "이용 없는 대여소");
    addStation(stations, "00207", "여의나루역 1번출구 앞");

    LinkedList<Trip> trips;
    fillTrips(trips);
    Graph graph(stations);
    Graph::BuildStats stats;
    graph.buildGraph(trips, stats);

    DynamicArray<const Station*> arr;
    buildStationPtrArray(stations, arr);
    check(ids(arr, 4) == "00102,00207,00393,00500", "대여소 포인터 배열: 트리 중위 순회 = 번호 순");
    check(arr[0] == stations.find("00102"), "배열에는 트리 노드 안 대여소 주소가 그대로 들어감");

    std::size_t n = topNStations(arr, StationKey::RentCount, 2);
    check(n == 2 && ids(arr, 2) == "00393,00102" && arr[0]->rentCount == 3,
          "대여 횟수 TOP-2: 00393(3), 00102(2)");

    sortStations(arr, StationKey::TotalCount, true);
    check(ids(arr, 4) == "00393,00102,00207,00500", "이용 횟수(대여+반납) 내림차순: 5, 4, 3, 0");

    sortStations(arr, StationKey::ReturnCount, true);
    check(ids(arr, 4) == "00102,00207,00393,00500", "반납 횟수 동점(2) → 대여소번호 순");

    sortStations(arr, StationKey::OutDegree, true);
    check(ids(arr, 4) == "00102,00393,00207,00500", "진출 차수 내림차순, 동점은 번호 순 (직전 정렬과 무관)");

    sortStations(arr, StationKey::InDegree, false);
    check(ids(arr, 4) == "00500,00102,00207,00393", "진입 차수 오름차순: 0, 1, 2, 2");

    check(topNStations(arr, StationKey::RentCount, 0) == 0, "대여소 TOP-0 → 0개");
    check(topNStations(arr, StationKey::RentCount, 100) == 4, "N 이 대여소 수보다 크면 전체 개수");

    DynamicArray<const Station*> empty;
    StationTree noStations;
    buildStationPtrArray(noStations, empty);
    check(empty.empty() && topNStations(empty, StationKey::TotalCount, 3) == 0, "대여소가 없으면 0개");

    topNStations(arr, StationKey::RentCount, 1);
    std::ostringstream out;
    printStations(arr, 0, 1, out);
    check(out.str() == "1. 00393 동대문역 8번 출구  대여 3 / 반납 2 / 진출 2 / 진입 2\n",
          "대여소 출력: 번호, 이름, 대여·반납 횟수, 진출·진입 차수");

    std::string names;
    for (int k = 0; k < STATION_KEY_COUNT; ++k) {
        if (stationKeyName(static_cast<StationKey>(k))[0] == '\0') names = "빈 이름";
    }
    check(names.empty(), "대여소 기준 이름이 모두 있음");
}

}  // namespace

int main() {
    testMergeSortBasic();
    testBinarySearchBasic();
    testTripSort();
    testTopN();
    testTripSearch();
    testPrint();
    testStationSort();
    testSampleCsv();

    std::printf("\n%s (실패 %d)\n", failures == 0 ? "모두 통과" : "실패 있음", failures);
    return failures == 0 ? 0 : 1;
}
