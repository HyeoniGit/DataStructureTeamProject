#include "modules/Analysis.h"

#include "modules/BikeIndex.h"   // formatDateTime

namespace {

// ----- 기준별 비교 함수 (오름차순: a 가 b 보다 작으면 true) -----
bool lessBikeId(const Trip* a, const Trip* b)        { return a->bikeId < b->bikeId; }
bool lessRentTime(const Trip* a, const Trip* b)      { return a->rentTime < b->rentTime; }
bool lessRentStation(const Trip* a, const Trip* b)   { return a->rentStationId < b->rentStationId; }
bool lessReturnTime(const Trip* a, const Trip* b)    { return a->returnTime < b->returnTime; }
bool lessReturnStation(const Trip* a, const Trip* b) { return a->returnStationId < b->returnStationId; }
bool lessMinutes(const Trip* a, const Trip* b)       { return a->useMinutes < b->useMinutes; }
bool lessDistance(const Trip* a, const Trip* b)      { return a->distanceM < b->distanceM; }

typedef bool (*TripLess)(const Trip*, const Trip*);

TripLess lessFor(TripKey key) {
    switch (key) {
        case TripKey::BikeId:        return lessBikeId;
        case TripKey::RentTime:      return lessRentTime;
        case TripKey::RentStation:   return lessRentStation;
        case TripKey::ReturnTime:    return lessReturnTime;
        case TripKey::ReturnStation: return lessReturnStation;
        case TripKey::Minutes:       return lessMinutes;
        case TripKey::Distance:      return lessDistance;
    }
    return lessRentTime;
}

// ----- 이분 탐색에서 쓰는 키 꺼내기 -----

// 숫자 기준 → double (일시 14자리도 double 로 정확히 표현됨: 2^53 ≈ 9 × 10^15)
double numberKey(const Trip* t, TripKey key) {
    switch (key) {
        case TripKey::RentTime:   return static_cast<double>(t->rentTime);
        case TripKey::ReturnTime: return static_cast<double>(t->returnTime);
        case TripKey::Minutes:    return static_cast<double>(t->useMinutes);
        case TripKey::Distance:   return t->distanceM;
        default:                  return 0.0;
    }
}

const std::string& stringKey(const Trip* t, TripKey key) {
    switch (key) {
        case TripKey::RentStation:   return t->rentStationId;
        case TripKey::ReturnStation: return t->returnStationId;
        default:                     return t->bikeId;
    }
}

std::string stationLabel(const StationTree& stations, const std::string& id) {
    const Station* s = stations.find(id);
    if (s == nullptr) return id;
    return s->name;
}

}  // namespace

const char* tripKeyName(TripKey key) {
    switch (key) {
        case TripKey::BikeId:        return "자전거번호";
        case TripKey::RentTime:      return "대여일시";
        case TripKey::RentStation:   return "대여 대여소번호";
        case TripKey::ReturnTime:    return "반납일시";
        case TripKey::ReturnStation: return "반납대여소번호";
        case TripKey::Minutes:       return "이용시간(분)";
        case TripKey::Distance:      return "이용거리(M)";
    }
    return "";
}

bool isStringKey(TripKey key) {
    return key == TripKey::BikeId || key == TripKey::RentStation || key == TripKey::ReturnStation;
}

void buildTripPtrArray(const LinkedList<Trip>& trips, DynamicArray<const Trip*>& out) {
    out.clear();
    out.reserve(trips.size());
    for (const auto* node = trips.head(); node != nullptr; node = node->next) {
        out.pushBack(&node->data);
    }
}

void sortTrips(DynamicArray<const Trip*>& arr, TripKey key, bool descending) {
    TripLess less = lessFor(key);
    if (descending) {
        // b < a 이면 a 가 앞. 같은 값이면 false → 원래 순서 유지 (안정성 그대로)
        mergeSort(arr, [less](const Trip* a, const Trip* b) { return less(b, a); });
    } else {
        mergeSort(arr, less);
    }
}

std::size_t topN(DynamicArray<const Trip*>& arr, TripKey key, std::size_t n) {
    sortTrips(arr, key, true);
    return n < arr.size() ? n : arr.size();
}

bool findTripsInRange(const DynamicArray<const Trip*>& arr, TripKey key,
                      double from, double to, std::size_t& begin, std::size_t& end) {
    begin = end = 0;
    if (isStringKey(key)) return false;      // 숫자 기준만

    auto getKey = [key](const Trip* t) { return numberKey(t, key); };
    begin = lowerBound(arr, from, getKey);   // from 이상인 첫 위치
    end   = upperBound(arr, to, getKey);     // to 보다 큰 첫 위치
    if (end < begin) end = begin;            // from > to 로 들어온 경우
    return begin < end;
}

bool findTripsById(const DynamicArray<const Trip*>& arr, TripKey key,
                   const std::string& id, std::size_t& begin, std::size_t& end) {
    begin = end = 0;
    if (!isStringKey(key)) return false;     // 문자열 기준만

    auto getKey = [key](const Trip* t) -> const std::string& { return stringKey(t, key); };
    begin = lowerBound(arr, id, getKey);
    end   = upperBound(arr, id, getKey);
    return begin < end;
}

const Trip* findTripExact(const DynamicArray<const Trip*>& arr, TripKey key, double value) {
    if (isStringKey(key)) return nullptr;    // 숫자 기준만

    std::size_t index = 0;
    bool found = binarySearch(arr, value,
                              [key](const Trip* t) { return numberKey(t, key); },
                              index);
    return found ? arr[index] : nullptr;
}

const Trip* findTripExact(const DynamicArray<const Trip*>& arr, TripKey key, const std::string& id) {
    if (!isStringKey(key)) return nullptr;   // 문자열 기준만

    std::size_t index = 0;
    bool found = binarySearch(arr, id,
                              [key](const Trip* t) -> const std::string& { return stringKey(t, key); },
                              index);
    return found ? arr[index] : nullptr;
}

void printTrips(const DynamicArray<const Trip*>& arr, std::size_t begin, std::size_t end,
                const StationTree& stations, std::ostream& out) {
    if (end > arr.size()) end = arr.size();
    for (std::size_t i = begin; i < end; ++i) {
        const Trip* t = arr[i];
        out << (i - begin + 1) << ". " << t->bikeId << "  "
            << formatDateTime(t->rentTime) << " " << stationLabel(stations, t->rentStationId)
            << " -> "
            << formatDateTime(t->returnTime) << " " << stationLabel(stations, t->returnStationId)
            << "  (" << t->useMinutes << "분, " << static_cast<long long>(t->distanceM) << "m)\n";
    }
}

// ===================== 대여소 전용 =====================

namespace {

// 기준값 꺼내기 (TotalCount 는 int 두 개의 합이라 long long 으로)
long long stationValue(const Station* s, StationKey key) {
    switch (key) {
        case StationKey::RentCount:   return s->rentCount;
        case StationKey::ReturnCount: return s->returnCount;
        case StationKey::TotalCount:  return static_cast<long long>(s->rentCount) + s->returnCount;
        case StationKey::OutDegree:   return s->outDegree;
        case StationKey::InDegree:    return s->inDegree;
    }
    return 0;
}

}  // namespace

const char* stationKeyName(StationKey key) {
    switch (key) {
        case StationKey::RentCount:   return "대여 횟수";
        case StationKey::ReturnCount: return "반납 횟수";
        case StationKey::TotalCount:  return "이용 횟수(대여+반납)";
        case StationKey::OutDegree:   return "진출 차수";
        case StationKey::InDegree:    return "진입 차수";
    }
    return "";
}

void buildStationPtrArray(const StationTree& tree, DynamicArray<const Station*>& out) {
    out.clear();
    out.reserve(tree.size());
    tree.inorder([&out](const std::string&, const Station& s) { out.pushBack(&s); });
}

void sortStations(DynamicArray<const Station*>& arr, StationKey key, bool descending) {
    // 같은 값이면 대여소번호가 작은 쪽이 앞 → 직전에 어떤 기준으로 정렬했든 결과가 항상 같다
    mergeSort(arr, [key, descending](const Station* a, const Station* b) {
        long long va = stationValue(a, key);
        long long vb = stationValue(b, key);
        if (va != vb) return descending ? vb < va : va < vb;
        return a->id < b->id;
    });
}

std::size_t topNStations(DynamicArray<const Station*>& arr, StationKey key, std::size_t n) {
    sortStations(arr, key, true);
    return n < arr.size() ? n : arr.size();
}

void printStations(const DynamicArray<const Station*>& arr, std::size_t begin, std::size_t end,
                   std::ostream& out) {
    if (end > arr.size()) end = arr.size();
    for (std::size_t i = begin; i < end; ++i) {
        const Station* s = arr[i];
        out << (i - begin + 1) << ". " << s->id << " " << s->name
            << "  대여 " << s->rentCount << " / 반납 " << s->returnCount
            << " / 진출 " << s->outDegree << " / 진입 " << s->inDegree << "\n";
    }
}
