// 대여이력 CSV 파서 테스트 — 인코딩(UTF-8 / UTF-8 BOM / CP949), 줄바꿈(LF / CRLF), 형식 처리
// 저장소 루트에서 실행해야 함 (data/sample.csv 를 읽음):  make test
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "modules/CsvParser.h"

static int passed = 0, failed = 0;
static void check(bool ok, const char* label) {
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << label << '\n';
    if (ok) ++passed; else ++failed;
}

static std::string readAll(const char* path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}
static void writeAll(const char* path, const std::string& bytes) {
    std::ofstream out(path, std::ios::binary);
    out << bytes;
}

static bool sameTrip(const Trip& a, const Trip& b) {
    return a.bikeId == b.bikeId && a.rentTime == b.rentTime && a.returnTime == b.returnTime &&
           a.rentStationId == b.rentStationId && a.returnStationId == b.returnStationId &&
           a.useMinutes == b.useMinutes && a.distanceM == b.distanceM;
}
// 두 목록이 길이·내용 모두 같은지
static bool sameTrips(const LinkedList<Trip>& a, const LinkedList<Trip>& b) {
    if (a.size() != b.size()) return false;
    auto* x = a.head();
    auto* y = b.head();
    while (x != nullptr && y != nullptr) {
        if (!sameTrip(x->data, y->data)) return false;
        x = x->next;
        y = y->next;
    }
    return true;
}
static bool sameResult(const ParseResult& a, const ParseResult& b) {
    return a.lines == b.lines && a.loaded == b.loaded && a.skipped == b.skipped;
}

// data/sample.csv 앞 3줄(헤더 + 2건)을 CP949 로 변환한 바이트
// (원본 대여이력 파일은 CP949. 대여소명 "서울체육고등학교 앞" 등 한글이 2바이트씩 들어 있음)
static const char* CP949_HEAD3 =
    "\"\xc0\xda\xc0\xfc\xb0\xc5\xb9\xf8\xc8\xa3\",\"\xb4\xeb\xbf\xa9\xc0\xcf\xbd\xc3\",\"\xb4\xeb\xbf\xa9 \xb4\xeb\xbf\xa9\xbc\xd2\xb9\xf8\xc8\xa3\",\"\xb4\xeb\xbf\xa9 \xb4\xeb\xbf\xa9\xbc\xd2\xb8\xed\",\"\xb4\xeb\xbf\xa9\xb0\xc5\xc4\xa1\xb4\xeb\",\"\xb9\xdd\xb3\xb3\xc0\xcf\xbd\xc3\",\"\xb9\xdd\xb3\xb3\xb4\xeb\xbf\xa9\xbc\xd2\xb9\xf8\xc8\xa3\",\"\xb9\xdd\xb3\xb3\xb4\xeb\xbf\xa9\xbc\xd2\xb8\xed\",\"\xb9\xdd\xb3\xb3\xb0\xc5\xc4\xa1\xb4\xeb\",\"\xc0\xcc\xbf\xeb\xbd\xc3\xb0\xa3(\xba\xd0)\",\"\xc0\xcc\xbf\xeb\xb0\xc5\xb8\xae(M)\",\"\xbb\xfd\xb3\xe2\",\"\xbc\xba\xba\xb0\",\"\xc0\xcc\xbf\xeb\xc0\xda\xc1\xbe\xb7\xf9\",\"\xb4\xeb\xbf\xa9\xb4\xeb\xbf\xa9\xbc\xd2ID\",\"\xb9\xdd\xb3\xb3\xb4\xeb\xbf\xa9\xbc\xd2ID\"\x0d\n"
    "\"SPB-58380\",\"2026-06-01 00:01:33\",\"01291\",\"\xbc\xad\xbf\xef\xc3\xbc\xc0\xb0\xb0\xed\xb5\xee\xc7\xd0\xb1\xb3 \xbe\xd5\",\"0\",\"2026-06-01 00:01:42\",\"01291\",\"\xbc\xad\xbf\xef\xc3\xbc\xc0\xb0\xb0\xed\xb5\xee\xc7\xd0\xb1\xb3 \xbe\xd5\",\"0\",\"0\",\"0.00\",\"2007\",\"F\",\"\xb3\xbb\xb1\xb9\xc0\xce\",\"ST-1580\",\"ST-1580\"\x0d\n"
    "\"SPB-67718\",\"2026-06-01 00:05:40\",\"01416\",\"\xbb\xf3\xba\xc0\xbf\xaa 3\xb9\xf8\xc3\xe2\xb1\xb8\",\"0\",\"2026-06-01 00:20:46\",\"04116\",\"\xc0\xcc\xb9\xae" "e\xc6\xed\xc7\xd1\xbc\xbc\xbb\xf3\xbe\xc6\xc6\xc4\xc6\xae \xbe\xd5\",\"0\",\"15\",\"2845.18\",\"1994\",\"M\",\"\xb3\xbb\xb1\xb9\xc0\xce\",\"ST-850\",\"ST-2505\"\x0d\n";

int main() {
    // ---- 기준: UTF-8 (sample.csv) ----
    LinkedList<Trip> base;
    ParseResult baseResult;
    check(loadTrips("data/sample.csv", base, baseResult), "UTF-8: open data/sample.csv");
    check(baseResult.loaded > 0 && baseResult.loaded + baseResult.skipped == baseResult.lines,
          "UTF-8: loaded + skipped == lines");
    check(base.head() != nullptr && base.head()->data.rentStationId == "01291" &&
              base.head()->data.rentTime == 20260601000133LL,
          "UTF-8: first trip fields");

    const std::string utf8 = readAll("data/sample.csv");

    // ---- UTF-8 BOM (엑셀 "CSV UTF-8" 저장) ----
    const char* bomPath = "test_csv_tmp_bom.csv";
    writeAll(bomPath, "\xEF\xBB\xBF" + utf8);
    LinkedList<Trip> bom;
    ParseResult bomResult;
    loadTrips(bomPath, bom, bomResult);
    check(sameResult(bomResult, baseResult) && sameTrips(bom, base), "UTF-8 BOM: same as UTF-8");
    std::remove(bomPath);

    // ---- CRLF 줄바꿈 (윈도우에서 저장) ----
    std::string crlfBytes;
    for (char c : utf8) {
        if (c == '\n') crlfBytes += '\r';
        crlfBytes += c;
    }
    const char* crlfPath = "test_csv_tmp_crlf.csv";
    writeAll(crlfPath, crlfBytes);
    LinkedList<Trip> crlf;
    ParseResult crlfResult;
    loadTrips(crlfPath, crlf, crlfResult);
    check(sameResult(crlfResult, baseResult) && sameTrips(crlf, base), "CRLF: same as LF");
    std::remove(crlfPath);

    // ---- CP949 (원본 대여이력 인코딩) ----
    // CP949 둘째 바이트(0x41~0xFE)는 ',' '"' '\r' '\n' 과 겹치지 않으므로 칸 나누기가 깨지지 않는다.
    // Trip 에 담는 칸(번호·일시·숫자)은 ASCII 라 인코딩과 무관하게 같은 값이 나와야 한다.
    const char* cpPath = "test_csv_tmp_cp949.csv";
    writeAll(cpPath, CP949_HEAD3);
    LinkedList<Trip> cp;
    ParseResult cpResult;
    loadTrips(cpPath, cp, cpResult);
    LinkedList<Trip> firstTwo;
    for (auto* n = base.head(); n != nullptr && firstTwo.size() < 2; n = n->next) firstTwo.pushBack(n->data);
    check(cpResult.lines == 2 && cpResult.loaded == 2 && cpResult.skipped == 0, "CP949: 2 rows loaded");
    check(sameTrips(cp, firstTwo), "CP949: same trips as UTF-8");
    std::remove(cpPath);

    // ---- 형식 처리 ----
    const char* fmtPath = "test_csv_tmp_format.csv";
    writeAll(fmtPath,
             "header\n"
             "\"SPB-1\",\"2026-06-01 00:01:33\",\"102\",\"a, b\",\"0\",\"2026-06-01 00:11:33\",\"393\",\"c\",\"0\",\"10\",\"1500.5\",\"\",\"\",\"\",\"\",\"\"\n"
             "\"SPB-2\",\"2026-06-01 00:01:33\",\"\",\"x\",\"0\",\"2026-06-01 00:11:33\",\"00393\",\"c\",\"0\",\"10\",\"0\",\"\",\"\",\"\",\"\",\"\"\n"
             "\"SPB-3\",\"2026/06/01 00:01\",\"00102\",\"x\",\"0\",\"2026-06-01 00:11:33\",\"00393\",\"c\",\"0\",\"10\",\"0\",\"\",\"\",\"\",\"\",\"\"\n"
             "\"SPB-4\",\"2026-06-01 00:01:33\",\"00102\",\"x\",\"0\",\"2026-06-01 00:11:33\",\"00393\",\"c\",\"0\",\"\",\"\",\"\",\"\",\"\",\"\",\"\"\n"
             "too,short\n"
             "\n");
    LinkedList<Trip> fmt;
    ParseResult fmtResult;
    loadTrips(fmtPath, fmt, fmtResult);
    std::remove(fmtPath);
    check(fmtResult.lines == 5 && fmtResult.loaded == 2 && fmtResult.skipped == 3,
          "format: 5 lines, 2 loaded, 3 skipped (empty id / bad date / too few columns)");
    check(fmt.head() != nullptr && fmt.head()->data.rentStationId == "00102" &&
              fmt.head()->data.returnStationId == "00393" && fmt.head()->data.useMinutes == 10 &&
              fmt.head()->data.distanceM == 1500.5,
          "format: quoted comma kept in one field, ids padded to 5 digits");
    check(fmt.size() == 2 && fmt.head()->next->data.useMinutes == 0 && fmt.head()->next->data.distanceM == 0.0,
          "format: empty minutes/distance -> 0");

    // ---- 보조 함수 ----
    check(parseDateTime("2026-06-01 00:01:33") == 20260601000133LL, "parseDateTime ok");
    check(parseDateTime(" 2026-06-01 00:01:33\r") == 20260601000133LL, "parseDateTime trims");
    check(parseDateTime("2026/06/01 00:01:33") == -1 && parseDateTime("") == -1, "parseDateTime rejects bad format");
    check(normalizeStationId(" 102") == "00102" && normalizeStationId("00393") == "00393", "normalizeStationId pads");
    check(normalizeStationId("") == "" && normalizeStationId("ST-1") == "" && normalizeStationId("123456") == "",
          "normalizeStationId rejects invalid");

    LinkedList<Trip> none;
    ParseResult noneResult;
    check(!loadTrips("data/__no_such_file__.csv", none, noneResult), "missing file -> false");

    std::cout << "RESULT: " << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
