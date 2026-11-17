// 정렬 · TOP-N · 이분 탐색 실제 데이터 측정 (make test 에는 포함되지 않음)
// 실행: make benchmark-analysis CSV="data/서울특별시 공공자전거 대여이력 정보_2606.csv"
#include <chrono>
#include <cstdio>
#include <string>

#include "core/DynamicArray.h"
#include "core/LinkedList.h"
#include "modules/Analysis.h"
#include "modules/CsvParser.h"

namespace {

double secondsSince(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("사용법: %s <대여이력 CSV>\n", argv[0]);
        return 1;
    }

    auto start = std::chrono::steady_clock::now();
    LinkedList<Trip> trips;
    ParseResult result;
    if (!loadTrips(argv[1], trips, result)) {
        std::printf("파일을 열 수 없음: %s\n", argv[1]);
        return 1;
    }
    std::printf("읽기: %lld건 (건너뜀 %lld) %.2f초\n", result.loaded, result.skipped, secondsSince(start));

    DynamicArray<const Trip*> arr;
    start = std::chrono::steady_clock::now();
    buildTripPtrArray(trips, arr);
    std::printf("포인터 배열: %zu개 %.3f초\n\n", arr.size(), secondsSince(start));

    const TripKey keys[TRIP_KEY_COUNT] = {
        TripKey::BikeId, TripKey::RentTime, TripKey::RentStation, TripKey::ReturnTime,
        TripKey::ReturnStation, TripKey::Minutes, TripKey::Distance};

    std::printf("%-16s %10s %10s\n", "기준", "오름차순", "내림차순");
    for (TripKey key : keys) {
        start = std::chrono::steady_clock::now();
        sortTrips(arr, key, false);
        double asc = secondsSince(start);
        start = std::chrono::steady_clock::now();
        sortTrips(arr, key, true);
        double desc = secondsSince(start);
        std::printf("%-16s %9.2f초 %9.2f초\n", tripKeyName(key), asc, desc);
    }

    // 이분 탐색: 정렬 후 6월 1일 08시대 대여 기록
    sortTrips(arr, TripKey::RentTime, false);
    std::size_t b = 0, e = 0;
    start = std::chrono::steady_clock::now();
    const int REPEAT = 100000;
    for (int i = 0; i < REPEAT; ++i)
        findTripsInRange(arr, TripKey::RentTime, 20260601080000.0, 20260601085959.0, b, e);
    double perSearch = secondsSince(start) / REPEAT;
    std::printf("\n기간 검색 06-01 08시대: %zu건, 1회 %.2f마이크로초\n", e - b, perSearch * 1e6);

    // 비교: 같은 조건 순차 탐색
    start = std::chrono::steady_clock::now();
    std::size_t count = 0;
    for (std::size_t i = 0; i < arr.size(); ++i)
        if (arr[i]->rentTime >= 20260601080000 && arr[i]->rentTime <= 20260601085959) ++count;
    std::printf("순차 탐색 같은 조건: %zu건, 1회 %.2f밀리초\n", count, secondsSince(start) * 1e3);

    std::size_t n = topN(arr, TripKey::Minutes, 5);
    std::printf("\nTOP-5 이용시간:");
    for (std::size_t i = 0; i < n; ++i) std::printf(" %d분", arr[i]->useMinutes);
    std::printf("\n");
    return 0;
}
