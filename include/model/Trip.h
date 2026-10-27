#ifndef MODEL_TRIP_H
#define MODEL_TRIP_H

#include <string>

// 대여이력 CSV 한 줄 (16개 컬럼 중 사용하는 것만)
// - 대여소번호는 앞자리 0 때문에 문자열 ("00393")
// - 일시는 "YYYY-MM-DD HH:MM:SS" 문자열 그대로 저장
//   → 형식이 고정 길이라 문자열 비교 순서 = 시간 순서 (정렬·이분 탐색에 바로 사용)
struct Trip {
    std::string bikeId;             // 자전거번호        "SPB-58380"
    std::string rentTime;           // 대여일시          "2026-06-01 00:01:33"
    std::string rentStationId;      // 대여 대여소번호   "01291"
    std::string rentStationName;    // 대여 대여소명
    std::string returnTime;         // 반납일시
    std::string returnStationId;    // 반납대여소번호
    std::string returnStationName;  // 반납대여소명
    int         useMinutes = 0;     // 이용시간(분)
    double      distanceM  = 0.0;   // 이용거리(M) — 필요 시 사용
};

// CSV 컬럼 위치 (0부터, 헤더 기준)
namespace TripCol {
    const int BIKE_ID             = 0;
    const int RENT_TIME           = 1;
    const int RENT_STATION_ID     = 2;
    const int RENT_STATION_NAME   = 3;
    const int RETURN_TIME         = 5;
    const int RETURN_STATION_ID   = 6;
    const int RETURN_STATION_NAME = 7;
    const int USE_MINUTES         = 9;
    const int DISTANCE_M          = 10;
    const int COUNT               = 16;  // 전체 컬럼 수 (파싱 검증용)
}

#endif
