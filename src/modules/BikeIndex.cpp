#include "modules/BikeIndex.h"

namespace {

void appendTwoDigits(std::string& s, long long v) {
    s += static_cast<char>('0' + (v / 10) % 10);
    s += static_cast<char>('0' + v % 10);
}

// 대여소번호 → 이름. 대여소 정보에 없는 번호면 번호 그대로
std::string stationLabel(const StationTree& stations, const std::string& id) {
    const Station* s = stations.find(id);
    if (s == nullptr) return id + "(대여소 정보 없음)";
    return s->name;
}

}  // namespace

std::string formatDateTime(long long t) {
    if (t <= 0) return "--";
    long long minute = (t / 100) % 100;
    long long hour   = (t / 10000) % 100;
    long long day    = (t / 1000000) % 100;
    long long month  = (t / 100000000) % 100;
    std::string s;
    appendTwoDigits(s, month);
    s += '-';
    appendTwoDigits(s, day);
    s += ' ';
    appendTwoDigits(s, hour);
    s += ':';
    appendTwoDigits(s, minute);
    return s;
}

long long buildBikeTree(const LinkedList<Trip>& trips, BikeTree& tree) {
    long long added = 0;
    for (const LinkedList<Trip>::Node* n = trips.head(); n != nullptr; n = n->next) {
        const Trip& trip = n->data;
        if (trip.bikeId.empty()) continue;

        BikeRoute* route = tree.find(trip.bikeId);       // 1. 이미 있는 자전거인가?
        if (route == nullptr) {                           // 2. 처음 보면 노드 새로 만들기
            BikeRoute fresh;
            fresh.bikeId = trip.bikeId;
            route = tree.insert(trip.bikeId, fresh);
        }
        route->trips.pushBack(&trip);                     // 3. 기록 주소를 리스트 끝에 추가
        ++added;
    }
    return added;
}

bool printBikeRoute(const BikeTree& bikes, const StationTree& stations,
                    const std::string& bikeId, std::ostream& out) {
    const BikeRoute* route = bikes.find(bikeId);
    if (route == nullptr) {
        out << bikeId << ": 찾을 수 없는 자전거입니다.\n";
        return false;
    }

    out << bikeId << " 이동 경로 (총 " << route->trips.size() << "회)\n";
    int i = 1;
    for (const LinkedList<const Trip*>::Node* n = route->trips.head(); n != nullptr; n = n->next, ++i) {
        const Trip* t = n->data;
        out << i << ". " << formatDateTime(t->rentTime) << "  "
            << stationLabel(stations, t->rentStationId) << " -> "
            << stationLabel(stations, t->returnStationId)
            << " (" << t->useMinutes << "분)\n";
    }
    return true;
}
