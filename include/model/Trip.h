#ifndef MODEL_TRIP_H
#define MODEL_TRIP_H

#include <string>

// 대여 기록 1건 (대여이력 CSV 한 줄)
// - 원본 6월 데이터가 약 418만 건이라 필요한 컬럼만 담는다.
// - 대여소명은 넣지 않는다. 대여소번호로 Station 에서 찾으면 되고,
//   한글 이름을 기록마다 복사하면 메모리가 수백 MB 더 든다.
// - 일시는 "2026-06-01 00:01:33" → 20260601000133 처럼 숫자로 저장.
//   크기 비교가 시간 순서와 같아서 정렬·이분 탐색에 바로 쓸 수 있다.
struct Trip {
    std::string bikeId;          // 자전거번호 "SPB-58380"
    long long   rentTime;        // 대여일시 (YYYYMMDDhhmmss)
    long long   returnTime;      // 반납일시 (YYYYMMDDhhmmss)
    std::string rentStation;     // 대여 대여소번호 "00393" (5자리 문자열, 앞자리 0 유지)
    std::string returnStation;   // 반납 대여소번호 "00393"
    int         minutes;         // 이용시간(분)
    double      distance;        // 이용거리(M)

    Trip() : rentTime(0), returnTime(0), minutes(0), distance(0.0) {}
};

#endif
