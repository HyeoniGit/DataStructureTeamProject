#include "modules/CsvParser.h"

#include <cstdlib>
#include <fstream>

namespace {

// 대여이력 CSV 컬럼 위치
const int COL_BIKE_ID        = 0;   // 자전거번호
const int COL_RENT_TIME      = 1;   // 대여일시
const int COL_RENT_STATION   = 2;   // 대여 대여소번호
const int COL_RETURN_TIME    = 5;   // 반납일시
const int COL_RETURN_STATION = 6;   // 반납대여소번호
const int COL_MINUTES        = 9;   // 이용시간(분)
const int COL_DISTANCE       = 10;  // 이용거리(M)
const int MAX_COLUMNS        = 16;

bool isDigit(char c) { return c >= '0' && c <= '9'; }

// 앞뒤 공백 제거
std::string trim(const std::string& s) {
    std::size_t begin = 0;
    std::size_t end = s.size();
    while (begin < end && (s[begin] == ' ' || s[begin] == '\t')) ++begin;
    while (end > begin && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r')) --end;
    return s.substr(begin, end - begin);
}

// 한 줄을 콤마로 나눠 fields 에 담고 칸 수를 반환한다.
// 따옴표로 감싼 칸은 따옴표를 떼고, 안의 콤마는 구분자로 보지 않는다. ("" 는 " 하나)
int splitCsvLine(const std::string& line, std::string fields[], int maxFields) {
    int count = 0;
    std::string cur;
    bool inQuotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    cur += '"';
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                cur += c;
            }
        } else if (c == '"') {
            inQuotes = true;
        } else if (c == ',') {
            if (count < maxFields) fields[count] = cur;
            ++count;
            cur.clear();
        } else if (c != '\r') {
            cur += c;
        }
    }
    if (count < maxFields) fields[count] = cur;
    ++count;
    return count;
}

// 정수 변환. 빈 값이거나 숫자가 아니면 false
bool toInt(const std::string& text, int& out) {
    if (text.empty()) return false;
    char* end = nullptr;
    long value = std::strtol(text.c_str(), &end, 10);
    if (*end != '\0') return false;
    out = static_cast<int>(value);
    return true;
}

// 실수 변환. 빈 값이거나 숫자가 아니면 false
bool toDouble(const std::string& text, double& out) {
    if (text.empty()) return false;
    char* end = nullptr;
    double value = std::strtod(text.c_str(), &end);
    if (*end != '\0') return false;
    out = value;
    return true;
}

}  // namespace

long long parseDateTime(const std::string& text) {
    // 형식: YYYY-MM-DD hh:mm:ss (19자)
    const std::string s = trim(text);
    if (s.size() != 19) return -1;

    long long value = 0;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (i == 4 || i == 7) {
            if (s[i] != '-') return -1;
        } else if (i == 10) {
            if (s[i] != ' ') return -1;
        } else if (i == 13 || i == 16) {
            if (s[i] != ':') return -1;
        } else {
            if (!isDigit(s[i])) return -1;
            value = value * 10 + (s[i] - '0');
        }
    }
    return value;
}

std::string normalizeStationId(const std::string& text) {
    std::string s = trim(text);
    if (s.empty() || s.size() > 5) return "";
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (!isDigit(s[i])) return "";
    }
    return std::string(5 - s.size(), '0') + s;
}

bool loadTrips(const std::string& path, LinkedList<Trip>& trips, ParseResult& result) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    std::string line;
    std::getline(file, line);  // 헤더 건너뛰기

    std::string fields[MAX_COLUMNS];
    while (std::getline(file, line)) {
        if (line.empty() || line == "\r") continue;
        ++result.lines;

        int count = splitCsvLine(line, fields, MAX_COLUMNS);
        if (count <= COL_DISTANCE) {
            ++result.skipped;
            continue;
        }

        Trip trip;
        trip.bikeId        = trim(fields[COL_BIKE_ID]);
        trip.rentTime      = parseDateTime(fields[COL_RENT_TIME]);
        trip.returnTime    = parseDateTime(fields[COL_RETURN_TIME]);
        trip.rentStation   = normalizeStationId(fields[COL_RENT_STATION]);
        trip.returnStation = normalizeStationId(fields[COL_RETURN_STATION]);

        // 그래프 간선을 못 만드는 기록은 건너뛴다 (대여소번호 빈 값, 일시 형식 오류)
        if (trip.bikeId.empty() || trip.rentStation.empty() || trip.returnStation.empty()
            || trip.rentTime < 0 || trip.returnTime < 0) {
            ++result.skipped;
            continue;
        }

        // 이용시간·이용거리는 비어 있으면 0 으로 둔다
        if (!toInt(trim(fields[COL_MINUTES]), trip.minutes)) trip.minutes = 0;
        if (!toDouble(trim(fields[COL_DISTANCE]), trip.distance)) trip.distance = 0.0;

        trips.pushBack(trip);
        ++result.loaded;
    }
    return true;
}
