#include "modules/CsvParser.h"

#include <cstdlib>
#include <fstream>
#include <istream>

namespace {

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

// 대여소 정보 CSV 칸 위치 (0부터)
namespace StationCol {
    const int ID        = 0;  // 대여소번호
    const int NAME      = 1;  // 보관소(대여소)명
    const int ADDRESS   = 3;  // 상세주소 (2: 자치구)
    const int LATITUDE  = 4;
    const int LONGITUDE = 5;
    const int LCD_RACKS = 7;  // LCD 거치대수
    const int QR_RACKS  = 8;  // QR 거치대수
    const int COUNT     = 11;
}

// CSV 레코드 하나를 읽는다. 따옴표 안에 줄바꿈이 있으면 다음 줄까지 이어 붙인다.
// (따옴표 개수가 홀수면 아직 따옴표 안 — "" 는 2개라 홀짝에 영향 없음)
bool readRecord(std::istream& in, std::string& record) {
    if (!std::getline(in, record)) return false;
    std::size_t quotes = 0;
    for (char c : record) if (c == '"') ++quotes;

    std::string next;
    while (quotes % 2 == 1 && std::getline(in, next)) {
        record += '\n';
        record += next;
        for (char c : next) if (c == '"') ++quotes;
    }
    return true;
}

// 칸 안의 줄바꿈을 공백으로 바꾸고 앞뒤 공백 제거 ("182-4\r\n(…)" → "182-4 (…)")
std::string oneLine(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '\r') continue;
        out += (c == '\n') ? ' ' : c;
    }
    return trim(out);
}

// 빈 칸이면 0
int toIntOrZero(const std::string& text) {
    int value = 0;
    return toInt(trim(text), value) ? value : 0;
}

double toDoubleOrZero(const std::string& text) {
    double value = 0.0;
    return toDouble(trim(text), value) ? value : 0.0;
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

    std::string fields[TripCol::COUNT];
    while (std::getline(file, line)) {
        if (line.empty() || line == "\r") continue;
        ++result.lines;

        int count = splitCsvLine(line, fields, TripCol::COUNT);
        if (count <= TripCol::DISTANCE_M) {
            ++result.skipped;
            continue;
        }

        Trip trip;
        trip.bikeId          = trim(fields[TripCol::BIKE_ID]);
        trip.rentTime        = parseDateTime(fields[TripCol::RENT_TIME]);
        trip.returnTime      = parseDateTime(fields[TripCol::RETURN_TIME]);
        trip.rentStationId   = normalizeStationId(fields[TripCol::RENT_STATION_ID]);
        trip.returnStationId = normalizeStationId(fields[TripCol::RETURN_STATION_ID]);

        // 그래프 간선을 못 만드는 기록은 건너뛴다 (대여소번호 빈 값, 일시 형식 오류)
        if (trip.bikeId.empty() || trip.rentStationId.empty() || trip.returnStationId.empty()
            || trip.rentTime < 0 || trip.returnTime < 0) {
            ++result.skipped;
            continue;
        }

        // 이용시간·이용거리는 비어 있으면 0 으로 둔다
        if (!toInt(trim(fields[TripCol::USE_MINUTES]), trip.useMinutes)) trip.useMinutes = 0;
        if (!toDouble(trim(fields[TripCol::DISTANCE_M]), trip.distanceM)) trip.distanceM = 0.0;

        trips.pushBack(trip);
        ++result.loaded;
    }
    return true;
}

bool isValidUtf8(const std::string& text) {
    std::size_t i = 0;
    const std::size_t n = text.size();
    while (i < n) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        std::size_t len = 0;
        if (c < 0x80) len = 1;
        else if (c >= 0xC2 && c <= 0xDF) len = 2;  // 한글은 3바이트 (0xE0~0xEF 로 시작)
        else if (c >= 0xE0 && c <= 0xEF) len = 3;
        else if (c >= 0xF0 && c <= 0xF4) len = 4;
        else return false;  // CP949 한글 첫 바이트(0xB0~0xC8)는 여기서 걸린다
        if (i + len > n) return false;
        for (std::size_t k = 1; k < len; ++k) {
            if ((static_cast<unsigned char>(text[i + k]) & 0xC0) != 0x80) return false;
        }
        i += len;
    }
    return true;
}

bool loadStations(const std::string& path, DynamicArray<Station>& stations, StationParseResult& result) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    std::string record;
    std::string fields[StationCol::COUNT];
    bool firstRecord = true;
    bool inData = false;  // 제목 줄을 지나 첫 대여소가 나왔는지

    while (readRecord(file, record)) {
        // 엑셀 "CSV UTF-8" 은 파일 맨 앞에 BOM(EF BB BF)을 붙인다
        if (firstRecord) {
            if (record.size() >= 3 && record.compare(0, 3, "\xEF\xBB\xBF") == 0) record.erase(0, 3);
            firstRecord = false;
        }
        if (trim(record).empty()) continue;

        const int count = splitCsvLine(record, fields, StationCol::COUNT);
        const std::string id = normalizeStationId(fields[StationCol::ID]);

        // 제목 줄: 대여소번호 칸이 숫자인 첫 줄이 나올 때까지 건너뛴다
        if (!inData) {
            if (id.empty()) continue;
            inData = true;
        }
        ++result.lines;

        if (id.empty() || count <= StationCol::QR_RACKS) {
            ++result.skipped;
            continue;
        }

        Station s;
        s.id        = id;
        s.name      = oneLine(fields[StationCol::NAME]);
        s.address   = oneLine(fields[StationCol::ADDRESS]);
        s.latitude  = toDoubleOrZero(fields[StationCol::LATITUDE]);
        s.longitude = toDoubleOrZero(fields[StationCol::LONGITUDE]);
        s.rackCount = toIntOrZero(fields[StationCol::LCD_RACKS]) + toIntOrZero(fields[StationCol::QR_RACKS]);

        if (!isValidUtf8(s.name) || !isValidUtf8(s.address)) ++result.notUtf8;

        stations.pushBack(s);
        ++result.loaded;
    }
    return true;
}
