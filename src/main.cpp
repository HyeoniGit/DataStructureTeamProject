// 따릉이 대여소 이동 연결망 분석 — 메뉴 (입력 → 결과 출력)
//
// 실행: ddareungi [대여소 정보 CSV] [대여이력 CSV]
//   인수가 없으면 data/sample_stations.csv, data/sample.csv
//
// 흐름
//   시작: 대여소 → 대여소 번호 트리 (1-1) → 대여이력 연결 리스트 (1-2) → 방향 가중 그래프 (2-1)
//         → 자전거 번호 트리 (2-3) → 정렬용 포인터 배열 (2-2)
//   메뉴: q 를 입력할 때까지 "옵션 선택 → 값 입력 → 결과 출력" 을 반복한다.
//         값을 묻는 중에는 빈 Enter 로 메뉴로 돌아간다 (q 는 메뉴에서만 종료).

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include <chrono>
#include <cstdio>
#include <cstddef>
#include <iostream>
#include <string>

#include "core/DynamicArray.h"
#include "core/LinkedList.h"
#include "model/Station.h"
#include "model/Trip.h"
#include "modules/Analysis.h"
#include "modules/BikeIndex.h"
#include "modules/CsvParser.h"
#include "modules/Graph.h"
#include "modules/StationIndex.h"
#include "modules/Traversal.h"

namespace {

const char* DEFAULT_STATIONS = "data/sample_stations.csv";
const char* DEFAULT_TRIPS    = "data/sample.csv";

const std::size_t PRINT_LIMIT = 20;   // 검색 결과는 앞에서 이만큼만 출력
const int         MAX_TOP_N   = 100;  // TOP-N 의 N 상한

// ===================== 출력 보조 =====================

// 4160957 → "4,160,957"
std::string withComma(long long value) {
    std::string digits = std::to_string(value < 0 ? -value : value);
    std::string out;
    int count = 0;
    for (std::size_t i = digits.size(); i > 0; --i) {
        out.insert(out.begin(), digits[i - 1]);
        if (++count % 3 == 0 && i > 1) out.insert(out.begin(), ',');
    }
    return value < 0 ? "-" + out : out;
}

// 경과 시간을 "1.23초" 형태로 (지수 표기 없이 소수 둘째 자리)
std::string secondsSince(std::chrono::steady_clock::time_point start) {
    double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f초", sec);
    return buf;
}

std::string stationName(const StationTree& stations, const std::string& id) {
    const Station* s = stations.find(id);
    return s == nullptr ? id : id + " " + s->name;
}

// 실행 인수로 받은 파일 경로를 화면에 보여 줄 때 쓴다.
// Windows 는 argv 를 시스템 코드 페이지(CP949)로 넘기는데 콘솔은 UTF-8 로 맞춰 두었으므로
// 한글 경로가 깨지지 않게 UTF-8 로 바꿔서 출력한다. (파일 열기는 원래 문자열 그대로 사용)
std::string displayPath(const std::string& path) {
#ifdef _WIN32
    int wlen = MultiByteToWideChar(CP_ACP, 0, path.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return path;
    std::wstring wide(static_cast<std::size_t>(wlen), L'\0');
    MultiByteToWideChar(CP_ACP, 0, path.c_str(), -1, &wide[0], wlen);
    int len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return path;
    std::string utf8(static_cast<std::size_t>(len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &utf8[0], len, nullptr, nullptr);
    utf8.resize(static_cast<std::size_t>(len - 1));   // 끝의 '\0' 제외
    return utf8;
#else
    return path;
#endif
}

// ===================== 입력 보조 =====================
// 모든 입력은 한 줄씩 받는다 (cin >> int 는 문자가 들어오면 실패 상태로 멈춤).

enum class Ask {
    Ok,     // 올바른 값을 받음
    Back,   // 빈 Enter → 메뉴로
    End     // 입력 끝 (Ctrl+Z, 입력 파일 끝) → 프로그램 종료
};

std::string trimmed(const std::string& s) {
    std::size_t b = 0, e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r')) ++b;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r')) --e;
    return s.substr(b, e - b);
}

// prompt 를 출력하고 한 줄 읽는다. 입력이 끝났으면 false
bool readLine(const std::string& prompt, std::string& line) {
    std::cout << prompt << " >> " << std::flush;
    if (!std::getline(std::cin, line)) {
        std::cout << "\n";
        return false;
    }
    line = trimmed(line);
    return true;
}

// 0 이상 정수만 (부호·소수점 없음)
bool toNumber(const std::string& text, long long& out) {
    if (text.empty() || text.size() > 12) return false;
    long long value = 0;
    for (char c : text) {
        if (c < '0' || c > '9') return false;
        value = value * 10 + (c - '0');
    }
    out = value;
    return true;
}

// lo ~ hi 사이 정수
Ask askNumber(const std::string& prompt, int lo, int hi, int& out) {
    std::string line;
    while (true) {
        if (!readLine(prompt + " (" + std::to_string(lo) + "~" + std::to_string(hi) + ")", line)) return Ask::End;
        if (line.empty()) return Ask::Back;
        long long value = 0;
        if (toNumber(line, value) && value >= lo && value <= hi) {
            out = static_cast<int>(value);
            return Ask::Ok;
        }
        std::cout << "  " << lo << "부터 " << hi << " 사이의 숫자를 입력하세요.\n";
    }
}

// 대여소번호: "102" → "00102". 대여소 번호 트리에 있는 번호만 받는다.
Ask askStation(const std::string& prompt, const StationTree& stations, std::string& id) {
    std::string line;
    while (true) {
        if (!readLine(prompt, line)) return Ask::End;
        if (line.empty()) return Ask::Back;
        id = normalizeStationId(line);
        if (id.empty()) {
            std::cout << "  대여소번호는 5자리 이하 숫자입니다. (예: 102, 00393)\n";
        } else if (stations.find(id) == nullptr) {
            std::cout << "  " << id << ": 대여소 정보에 없는 번호입니다.\n";
        } else {
            return Ask::Ok;
        }
    }
}

// 일시: "2026-06-01 08:00:00" / "2026-06-01 08:00" / "2026-06-01"
// 날짜만 쓰면 시작은 00:00:00, 끝은 23:59:59 로 본다.
Ask askDateTime(const std::string& prompt, bool isEnd, long long& out) {
    std::string line;
    while (true) {
        if (!readLine(prompt, line)) return Ask::End;
        if (line.empty()) return Ask::Back;
        std::string text = line;
        if (text.size() == 10) text += isEnd ? " 23:59:59" : " 00:00:00";
        else if (text.size() == 16) text += isEnd ? ":59" : ":00";
        out = parseDateTime(text);
        if (out >= 0) return Ask::Ok;
        std::cout << "  형식: 2026-06-01 08:00 (또는 2026-06-01, 2026-06-01 08:00:00)\n";
    }
}

// 자전거번호: "SPB-58380" 또는 숫자만 "58380"
Ask askBike(const std::string& prompt, std::string& id) {
    std::string line;
    if (!readLine(prompt, line)) return Ask::End;
    if (line.empty()) return Ask::Back;
    long long digits = 0;
    if (toNumber(line, digits)) {
        id = "SPB-" + line;
    } else {
        id = line;
        for (char& c : id) if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    }
    return Ask::Ok;
}

// ===================== 프로그램 상태 =====================

struct App {
    StationTree stations;                    // 1-1 대여소 번호 트리 (대여소 원본 보관)
    LinkedList<Trip> trips;                  // 1-2 대여이력 연결 리스트 (기록 원본 보관)
    BikeTree bikes;                          // 2-3 자전거 번호 트리
    DynamicArray<const Trip*> tripArr;       // 2-2 기록 주소 배열 (정렬·이분 탐색용)
    DynamicArray<const Station*> stationArr; // 2-2 대여소 주소 배열 (대여소 TOP-N 용)

    // tripArr 가 지금 어떤 기준으로 정렬돼 있는지 — 같은 기준이면 다시 정렬하지 않는다
    bool     tripSorted = false;
    TripKey  tripKey    = TripKey::RentTime;
    bool     tripDesc   = false;
};

void sortTripsOnce(App& app, TripKey key, bool descending) {
    if (app.tripSorted && app.tripKey == key && app.tripDesc == descending) return;
    auto start = std::chrono::steady_clock::now();
    sortTrips(app.tripArr, key, descending);
    app.tripSorted = true;
    app.tripKey = key;
    app.tripDesc = descending;
    if (app.tripArr.size() >= 100000) {
        std::cout << "  (" << tripKeyName(key) << " 기준 병합 정렬 " << secondsSince(start) << ")\n";
    }
}

// ===================== 시작: 자료구조 구축 =====================

bool lessStationId(const Station& a, const Station& b) { return a.id < b.id; }

bool buildAll(App& app, Graph& graph, const std::string& stationPath, const std::string& tripPath) {
    auto total = std::chrono::steady_clock::now();

    // 1-1 대여소 정보 → 대여소 번호 트리
    auto start = std::chrono::steady_clock::now();
    DynamicArray<Station> stationList;
    StationParseResult sr;
    if (!loadStations(stationPath, stationList, sr)) {
        std::cout << "대여소 정보 파일을 열 수 없습니다: " << displayPath(stationPath) << "\n";
        return false;
    }
    if (!isSortedById(stationList)) mergeSort(stationList, lessStationId);
    buildStationTree(stationList, app.stations);
    std::cout << "[1-1] 대여소 " << withComma(static_cast<long long>(app.stations.size())) << "곳"
              << " (건너뜀 " << sr.skipped << ", 트리 높이 " << app.stations.height() << ")  "
              << secondsSince(start) << "\n";
    if (sr.notUtf8 > 0) {
        std::cout << "      주의: 대여소명이 UTF-8 이 아닌 줄 " << sr.notUtf8
                  << "개 — 엑셀에서 \"CSV UTF-8\" 로 다시 저장하세요.\n";
    }

    // 1-2 대여이력 → 연결 리스트
    start = std::chrono::steady_clock::now();
    ParseResult tr;
    if (!loadTrips(tripPath, app.trips, tr)) {
        std::cout << "대여이력 파일을 열 수 없습니다: " << displayPath(tripPath) << "\n";
        return false;
    }
    std::cout << "[1-2] 대여 기록 " << withComma(tr.loaded) << "건"
              << " (전체 " << withComma(tr.lines) << "줄, 건너뜀 " << withComma(tr.skipped) << ")  "
              << secondsSince(start) << "\n";

    // 2-1 방향 가중 그래프
    start = std::chrono::steady_clock::now();
    Graph::BuildStats gs;
    graph.buildGraph(app.trips, gs);
    std::cout << "[2-1] 그래프 간선 " << withComma(gs.added) << "개"
              << " (누적 " << withComma(gs.updated) << ", 대여소 정보에 없음 " << withComma(gs.missingStation)
              << ", 잘못된 기록 " << withComma(gs.invalidTrip) << ")  " << secondsSince(start) << "\n";

    // 2-3 자전거 번호 트리
    start = std::chrono::steady_clock::now();
    buildBikeTree(app.trips, app.bikes);
    std::cout << "[2-3] 자전거 " << withComma(static_cast<long long>(app.bikes.size())) << "대  "
              << secondsSince(start) << "\n";

    // 2-2 정렬용 포인터 배열 (그래프를 만든 뒤라 대여소 횟수·차수가 채워져 있음)
    buildTripPtrArray(app.trips, app.tripArr);
    buildStationPtrArray(app.stations, app.stationArr);

    std::cout << "준비 완료 (" << secondsSince(total) << ")\n";
    return true;
}

// ===================== 메뉴 기능 =====================
// 각 함수는 입력이 끝났으면(Ask::End) false 를 돌려 프로그램을 끝낸다.

// 1. 대여소 검색 (BST)
bool menuStation(App& app) {
    std::string id;
    Ask a = askStation("대여소번호 (빈 Enter: 메뉴로)", app.stations, id);
    if (a != Ask::Ok) return a != Ask::End;

    const Station* s = app.stations.find(id);
    printStation(*s, std::cout);
    std::cout << "  진출 차수 " << s->outDegree << " (서로 다른 반납 대여소) / 진입 차수 " << s->inDegree
              << " (서로 다른 대여 대여소)\n";
    return true;
}

// 2. 자전거 이동 경로 (BST)
bool menuBike(App& app) {
    std::string id;
    Ask a = askBike("자전거번호 (예: SPB-58380 또는 58380, 빈 Enter: 메뉴로)", id);
    if (a != Ask::Ok) return a != Ask::End;

    // 이동이 많은 자전거(한 달 200회 이상도 있음)는 횟수를 먼저 보여 주고 전체 출력 여부를 묻는다
    const BikeRoute* route = app.bikes.find(id);
    if (route != nullptr && route->trips.size() > PRINT_LIMIT) {
        std::string line;
        if (!readLine(id + " 이동 " + withComma(static_cast<long long>(route->trips.size())) +
                          "회 — 전체 출력하려면 y (빈 Enter: 메뉴로)", line)) return false;
        if (line != "y" && line != "Y") return true;
    }
    printBikeRoute(app.bikes, app.stations, id, std::cout);
    return true;
}

// 3. 기간별 대여 기록 (병합 정렬 + 이분 탐색)
bool menuPeriod(App& app) {
    long long from = 0, to = 0;
    Ask a = askDateTime("시작 일시 (예: 2026-06-01 08:00, 빈 Enter: 메뉴로)", false, from);
    if (a != Ask::Ok) return a != Ask::End;
    a = askDateTime("끝 일시   (예: 2026-06-01 08:10)", true, to);
    if (a != Ask::Ok) return a != Ask::End;
    if (from > to) {
        std::cout << "  시작 일시가 끝 일시보다 늦습니다.\n";
        return true;
    }

    sortTripsOnce(app, TripKey::RentTime, false);
    std::size_t begin = 0, end = 0;
    if (!findTripsInRange(app.tripArr, TripKey::RentTime, static_cast<double>(from),
                          static_cast<double>(to), begin, end)) {
        std::cout << "  그 기간에 대여한 기록이 없습니다.\n";
        return true;
    }
    std::size_t count = end - begin;
    std::cout << "대여일시 " << formatDateTime(from) << " ~ " << formatDateTime(to) << ": "
              << withComma(static_cast<long long>(count)) << "건";
    if (count > PRINT_LIMIT) std::cout << " (앞 " << PRINT_LIMIT << "건만 출력)";
    std::cout << "\n";
    printTrips(app.tripArr, begin, count > PRINT_LIMIT ? begin + PRINT_LIMIT : end, app.stations, std::cout);
    return true;
}

// 4. 대여 기록 TOP-N (병합 정렬)
bool menuTripTop(App& app) {
    std::cout << "  1. 이용시간  2. 이용거리\n";
    int choice = 0, n = 0;
    Ask a = askNumber("기준 (빈 Enter: 메뉴로)", 1, 2, choice);
    if (a != Ask::Ok) return a != Ask::End;
    a = askNumber("N", 1, MAX_TOP_N, n);
    if (a != Ask::Ok) return a != Ask::End;

    TripKey key = choice == 1 ? TripKey::Minutes : TripKey::Distance;
    sortTripsOnce(app, key, true);
    std::size_t count = static_cast<std::size_t>(n) < app.tripArr.size() ? static_cast<std::size_t>(n)
                                                                          : app.tripArr.size();
    std::cout << tripKeyName(key) << " TOP-" << count << "\n";
    printTrips(app.tripArr, 0, count, app.stations, std::cout);
    return true;
}

// 5. 대여소 TOP-N (병합 정렬, 그래프에서 쌓은 횟수·차수)
bool menuStationTop(App& app) {
    for (int k = 0; k < STATION_KEY_COUNT; ++k) {
        std::cout << "  " << (k + 1) << ". " << stationKeyName(static_cast<StationKey>(k)) << "\n";
    }
    int choice = 0, n = 0;
    Ask a = askNumber("기준 (빈 Enter: 메뉴로)", 1, STATION_KEY_COUNT, choice);
    if (a != Ask::Ok) return a != Ask::End;
    a = askNumber("N", 1, MAX_TOP_N, n);
    if (a != Ask::Ok) return a != Ask::End;

    StationKey key = static_cast<StationKey>(choice - 1);
    std::size_t count = topNStations(app.stationArr, key, static_cast<std::size_t>(n));
    std::cout << "대여소 " << stationKeyName(key) << " TOP-" << count << "\n";
    printStations(app.stationArr, 0, count, std::cout);
    return true;
}

void printEdge(const StationTree& stations, const Graph& graph, const std::string& from, const std::string& to) {
    const Edge* e = graph.getEdge(from, to);
    std::cout << "  " << stationName(stations, from) << " -> " << stationName(stations, to) << ": ";
    if (e == nullptr) {
        std::cout << "이동 기록 없음\n";
    } else {
        char avg[32];
        std::snprintf(avg, sizeof(avg), "%.1f", Graph::averageMinutes(e));   // 16.9851 → 17.0
        std::cout << withComma(e->tripCount) << "회, 평균 " << avg << "분\n";
    }
}

// 6. 두 대여소 간 이동 (방향 가중 그래프)
bool menuEdge(App& app, const Graph& graph) {
    std::string a, b;
    Ask r = askStation("대여소 A 번호 (빈 Enter: 메뉴로)", app.stations, a);
    if (r != Ask::Ok) return r != Ask::End;
    r = askStation("대여소 B 번호", app.stations, b);
    if (r != Ask::Ok) return r != Ask::End;

    printEdge(app.stations, graph, a, b);
    if (a == b) {
        std::cout << "  (빌린 대여소에 그대로 반납한 기록)\n";
        return true;
    }
    printEdge(app.stations, graph, b, a);
    if (graph.isOneWay(a, b))      std::cout << "  편도: A -> B 만 있습니다.\n";
    else if (graph.isOneWay(b, a)) std::cout << "  편도: B -> A 만 있습니다.\n";
    else if (graph.getEdge(a, b) != nullptr) std::cout << "  양방향 이동이 있습니다.\n";
    return true;
}

// 7. 최소 간선 경로 (큐 + BFS)
bool menuPath(App& app, const Graph& graph) {
    std::string from, to;
    Ask a = askStation("출발 대여소번호 (빈 Enter: 메뉴로)", app.stations, from);
    if (a != Ask::Ok) return a != Ask::End;
    a = askStation("도착 대여소번호", app.stations, to);
    if (a != Ask::Ok) return a != Ask::End;

    DynamicArray<std::string> path;
    PathStats stats;
    PathResult r = shortestPath(graph, from, to, path, &stats);
    if (r == PathFound) printPath(path, app.stations, std::cout);
    else std::cout << "  " << pathResultMessage(r) << "\n";
    std::cout << "  (방문한 대여소 " << withComma(static_cast<long long>(stats.visitedCount)) << "곳)\n";
    return true;
}

void printMenu() {
    std::cout << "\n==== 따릉이 대여소 이동 연결망 ====\n"
              << " 1. 대여소 검색          5. 대여소 TOP-N\n"
              << " 2. 자전거 이동 경로      6. 두 대여소 간 이동\n"
              << " 3. 기간별 대여 기록      7. 최소 간선 경로 (BFS)\n"
              << " 4. 대여 기록 TOP-N       q. 종료\n";
}

}  // namespace

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);   // 콘솔 기본 코드 페이지(949)에서 UTF-8 한글이 깨지지 않도록
#endif

    std::string stationPath = argc >= 2 ? argv[1] : DEFAULT_STATIONS;
    std::string tripPath    = argc >= 3 ? argv[2] : DEFAULT_TRIPS;
    std::cout << "대여소 정보: " << displayPath(stationPath) << "\n대여이력:    " << displayPath(tripPath) << "\n\n";

    App app;
    Graph graph(app.stations);   // 간선은 대여소 번호 트리 안 Station 에 쌓인다
    if (!buildAll(app, graph, stationPath, tripPath)) {
        std::cout << "\n사용법: " << (argc > 0 ? argv[0] : "ddareungi")
                  << " [대여소 정보 CSV] [대여이력 CSV]\n";
        return 1;
    }

    std::string line;
    while (true) {
        printMenu();
        if (!readLine("선택", line)) break;   // 입력 끝 → 종료
        if (line == "q" || line == "Q") break;

        bool keepGoing = true;
        if      (line == "1") keepGoing = menuStation(app);
        else if (line == "2") keepGoing = menuBike(app);
        else if (line == "3") keepGoing = menuPeriod(app);
        else if (line == "4") keepGoing = menuTripTop(app);
        else if (line == "5") keepGoing = menuStationTop(app);
        else if (line == "6") keepGoing = menuEdge(app, graph);
        else if (line == "7") keepGoing = menuPath(app, graph);
        else if (!line.empty()) std::cout << "  메뉴에 없는 선택입니다. 1~7 또는 q 를 입력하세요.\n";
        if (!keepGoing) break;
    }
    std::cout << "종료합니다.\n";
    return 0;
}
