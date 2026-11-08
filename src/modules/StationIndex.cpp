#include "modules/StationIndex.h"

namespace {

// order[lo..hi] 의 가운데부터 넣고 왼쪽·오른쪽 절반을 재귀로 넣는다.
// 재귀 깊이는 log2(n) 정도라 대여소 2,789개면 12단계.
void insertMiddle(const DynamicArray<Station>& stations, const DynamicArray<std::size_t>& order,
                  long lo, long hi, StationTree& tree) {
    if (lo > hi) return;
    long mid = lo + (hi - lo) / 2;
    const Station& s = stations[order[static_cast<std::size_t>(mid)]];
    tree.insert(s.id, s);
    insertMiddle(stations, order, lo, mid - 1, tree);
    insertMiddle(stations, order, mid + 1, hi, tree);
}

}  // namespace

bool isSortedById(const DynamicArray<Station>& stations) {
    for (std::size_t i = 1; i < stations.size(); ++i) {
        if (stations[i].id < stations[i - 1].id) return false;
    }
    return true;
}

bool buildStationTree(const DynamicArray<Station>& sortedStations, StationTree& tree) {
    if (!isSortedById(sortedStations)) return false;

    // 같은 번호는 처음 것만 남긴 위치 목록 (정렬돼 있으므로 같은 번호는 붙어 있다)
    DynamicArray<std::size_t> order;
    order.reserve(sortedStations.size());
    for (std::size_t i = 0; i < sortedStations.size(); ++i) {
        if (sortedStations[i].id.empty()) continue;
        if (!order.empty() && sortedStations[order[order.size() - 1]].id == sortedStations[i].id) continue;
        order.pushBack(i);
    }

    insertMiddle(sortedStations, order, 0, static_cast<long>(order.size()) - 1, tree);
    return true;
}

void printStation(const Station& s, std::ostream& out) {
    out << "[" << s.id << "] " << s.name << "\n"
        << "  주소: " << s.address << "\n"
        << "  위치: " << s.latitude << ", " << s.longitude << "\n"
        << "  거치대 수: " << s.rackCount << "\n"
        << "  대여 " << s.rentCount << "회 / 반납 " << s.returnCount << "회\n";
}
