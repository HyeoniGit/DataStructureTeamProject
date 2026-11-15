// BST · 대여소 번호 트리 · 자전거 번호 트리 테스트
// 실행: make test
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

#include "core/BST.h"
#include "core/DynamicArray.h"
#include "core/LinkedList.h"
#include "modules/BikeIndex.h"
#include "modules/StationIndex.h"

namespace {

int failures = 0;

void check(bool ok, const char* what) {
    std::printf("[%s] %s\n", ok ? " OK " : "FAIL", what);
    if (!ok) ++failures;
}

std::string pad5(int n) {
    std::string s = std::to_string(n);
    while (s.size() < 5) s = "0" + s;
    return s;
}

Station makeStation(const std::string& id, const std::string& name) {
    Station s;
    s.id = id;
    s.name = name;
    return s;
}

Trip makeTrip(const std::string& bike, long long rent, const std::string& from,
              const std::string& to, int minutes) {
    Trip t;
    t.bikeId = bike;
    t.rentTime = rent;
    t.rentStationId = from;
    t.returnTime = rent + 1000;
    t.returnStationId = to;
    t.useMinutes = minutes;
    return t;
}

void testBasicBst() {
    BST<int, int> t;
    check(t.empty() && t.height() == 0 && t.find(1) == nullptr, "빈 트리: 크기 0, 높이 0, 검색 nullptr");

    int keys[] = {50, 30, 70, 20, 40, 60, 80};
    for (int k : keys) t.insert(k, k * 10);
    check(t.size() == 7 && t.height() == 3, "7개 삽입 후 크기 7, 높이 3");
    check(t.find(60) != nullptr && *t.find(60) == 600, "검색: 60 → 600");
    check(t.find(65) == nullptr, "없는 키 검색 → nullptr");

    int* before = t.find(40);
    int* again = t.insert(40, 999);
    check(again == before && *again == 400 && t.size() == 7, "같은 키 삽입: 새로 안 만들고 기존 주소 반환");

    std::string order;
    t.inorder([&](const int& k, int&) { order += std::to_string(k) + " "; });
    check(order == "20 30 40 50 60 70 80 ", "중위 순회: 오름차순");

    check(t.searchSteps(50) == 1 && t.searchSteps(20) == 3 && t.searchSteps(99) == -1, "비교 횟수 측정");
}

void testStationTree() {
    // 대여소 정보처럼 번호 오름차순, 중간에 빠진 번호와 중복 포함
    DynamicArray<Station> stations;
    int ids[] = {102, 103, 104, 105, 106, 107, 108, 111, 113, 114, 115, 117, 118, 119, 120};
    for (int id : ids) stations.pushBack(makeStation(pad5(id), "대여소" + std::to_string(id)));
    stations.pushBack(makeStation(pad5(120), "중복"));

    StationTree balanced;
    check(buildStationTree(stations, balanced), "정렬된 배열로 대여소 트리 만들기 성공");
    check(balanced.size() == 15, "중복 번호는 한 번만 저장 (15개)");
    check(balanced.height() == 4, "가운데부터 넣으면 높이 4 (= ceil(log2(16)))");
    check(balanced.find("00120") != nullptr && balanced.find("00120")->name == "대여소120", "중복이면 처음 것 유지");
    check(balanced.find("00109") == nullptr, "빠진 번호 검색 → nullptr");

    std::string order;
    balanced.inorder([&](const std::string& k, const Station&) { order += k + " "; });
    check(order.rfind("00102 00103", 0) == 0, "중위 순회 결과가 번호 순");

    // 비교: 위에서부터 그대로 넣으면 한쪽으로 쏠림
    StationTree naive;
    for (std::size_t i = 0; i < stations.size(); ++i) naive.insert(stations[i].id, stations[i]);
    check(naive.height() == 15, "그대로 넣으면 높이 15 (쏠림)");

    DynamicArray<Station> unsorted;
    unsorted.pushBack(makeStation("00200", "B"));
    unsorted.pushBack(makeStation("00100", "A"));
    StationTree t2;
    check(!buildStationTree(unsorted, t2) && t2.empty(), "정렬 안 된 배열은 거부");
}

void testBikeTree() {
    DynamicArray<Station> stations;
    stations.pushBack(makeStation("00103", "망원역 2번출구 앞"));
    stations.pushBack(makeStation("00113", "홍대입구역 2번출구 앞"));
    stations.pushBack(makeStation("00128", "신촌역(2호선) 1번출구 옆"));
    StationTree stationTree;
    buildStationTree(stations, stationTree);

    // 반납일시 순으로 들어온 대여이력 (자전거번호는 섞여 있음)
    LinkedList<Trip> trips;
    trips.pushBack(makeTrip("SPB-58380", 20260601081200, "00103", "00113", 12));
    trips.pushBack(makeTrip("SPB-43895", 20260601082000, "00113", "00128", 7));
    trips.pushBack(makeTrip("SPB-58380", 20260601184000, "00113", "00128", 9));
    trips.pushBack(makeTrip("", 20260601190000, "00103", "00103", 1));          // 자전거번호 빈 칸
    trips.pushBack(makeTrip("SPB-58380", 20260602075500, "00128", "99999", 21)); // 대여소 정보에 없는 번호

    BikeTree bikes;
    long long added = buildBikeTree(trips, bikes);
    check(added == 4, "자전거번호 빈 기록은 건너뜀 (4건 추가)");
    check(bikes.size() == 2, "자전거 2대 = 노드 2개");

    const BikeRoute* r = bikes.find("SPB-58380");
    check(r != nullptr && r->trips.size() == 3, "SPB-58380 이동 3회");
    check(r != nullptr && r->trips.head()->data == &trips.head()->data, "기록은 복사가 아니라 주소로 저장");

    bool ordered = true;
    long long prev = 0;
    for (auto* n = r->trips.head(); n != nullptr; n = n->next) {
        if (n->data->rentTime < prev) ordered = false;
        prev = n->data->rentTime;
    }
    check(ordered, "자전거별 기록이 시간순");

    std::ostringstream out;
    check(printBikeRoute(bikes, stationTree, "SPB-58380", out), "경로 출력 성공");
    std::string text = out.str();
    check(text.find("06-01 08:12  망원역 2번출구 앞 -> 홍대입구역 2번출구 앞 (12분)") != std::string::npos,
          "경로 출력: 시각, 대여소 이름, 이용시간");
    check(text.find("99999(대여소 정보 없음)") != std::string::npos, "대여소 정보에 없는 번호는 번호 그대로");
    std::cout << text;

    std::ostringstream none;
    check(!printBikeRoute(bikes, stationTree, "SPB-00000", none), "없는 자전거 → false");
}

void testLarge() {
    // 대여소 2,789개 규모: 그대로 넣기 vs 가운데부터 넣기
    DynamicArray<Station> stations;
    for (int i = 0; i < 2789; ++i) stations.pushBack(makeStation(pad5(102 + i), "S"));
    StationTree balanced;
    buildStationTree(stations, balanced);
    StationTree naive;
    for (std::size_t i = 0; i < stations.size(); ++i) naive.insert(stations[i].id, stations[i]);
    std::printf("       대여소 2789개: 그대로 넣기 높이 %d / 가운데부터 넣기 높이 %d\n",
                naive.height(), balanced.height());
    std::printf("       마지막 번호 검색 비교 횟수: %d / %d\n",
                naive.searchSteps(pad5(102 + 2788)), balanced.searchSteps(pad5(102 + 2788)));
    check(balanced.height() == 12 && naive.height() == 2789, "높이 12 vs 2789");

    // 자전거번호처럼 섞인 순서로 넣으면 그대로 넣어도 높이가 낮다
    BST<std::string, int> bikes;
    unsigned long long x = 12345;
    for (int i = 0; i < 40000; ++i) {
        x = x * 6364136223846793005ULL + 1442695040888963407ULL;
        bikes.insert("SPB-" + pad5(static_cast<int>((x >> 33) % 100000)), i);
    }
    std::printf("       섞인 순서 자전거번호 %zu개: 높이 %d\n", bikes.size(), bikes.height());
    check(bikes.height() < 60, "섞인 순서로 넣으면 쏠리지 않음");
}

}  // namespace

int main() {
    testBasicBst();
    testStationTree();
    testBikeTree();
    testLarge();
    std::printf("\n%s (실패 %d개)\n", failures == 0 ? "모든 테스트 통과" : "실패 있음", failures);
    return failures == 0 ? 0 : 1;
}
