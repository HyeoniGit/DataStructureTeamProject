// 대여소 정보 CSV 로더 테스트 (loadStations)
// 실제 파일(엑셀 "CSV UTF-8" 저장)과 같은 모양: BOM, 칸 안 줄바꿈이 있는 제목 5줄, 앞 공백 있는 대여소명,
// 주소 칸 안 줄바꿈, 빈 거치대 칸
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include "modules/CsvParser.h"
#include "modules/StationIndex.h"

static int passed = 0, failed = 0;
static void check(bool ok, const char* label) {
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << label << '\n';
    if (ok) ++passed; else ++failed;
}
static void writeAll(const char* path, const std::string& bytes) {
    std::ofstream out(path, std::ios::binary);
    out << bytes;
}
static std::string toCrlf(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '\n') out += '\r';
        out += c;
    }
    return out;
}

// 실제 파일 앞부분과 같은 구조 (제목 5줄 + 대여소)
static const std::string HEADER =
    "\xEF\xBB\xBF"
    "\"대여소\n번호\",보관소(대여소)명,소재지(위치),,,,\"설치\n시기\",설치형태,,\"운영\n방식\",\n"
    ",,,,,,,LCD,QR,,\n"
    ",,자치구,상세주소,위도,경도,,,,,\n"
    ",,,,,,,\"거치\n대수\",\"거치\n대수\",,\n"
    ",,,,,,,,,,\n";

static const std::string BODY =
    "00102, 망원역 1번출구 앞,마포구,서울특별시 마포구 월드컵로 72,37.5556488,126.9106293,2015-09-06,,15,QR,\n"
    "00393, 동대문역 8번 출구,종로구,서울특별시 종로구 종로 308,37.57014084,127.0093765,2018-01-26,12,,LCD,\n"
    "03651, 개포동역 4번출구,강남구,\"강남구 개포동 182-4\n(개포동역 4번출구)\",37.52,126.87,2019-01-01,5,5,QR,\n"
    "\"4592\",\"신길 \"\"우성\"\", 4차\",영등포구,영등포구 신길로,37.5,126.9,2020-01-01,,10,QR,\n"
    ",,,,,,,,,,\n"
    "ST-1,잘못된 번호,,,,,,,,,\n"
    "06190,짧은 줄\n"
    "\n";

int main() {
    const char* path = "test_station_tmp.csv";

    // ---- 기본 (LF) ----
    writeAll(path, HEADER + BODY);
    DynamicArray<Station> st;
    StationParseResult r;
    check(loadStations(path, st, r), "open file");
    check(r.loaded == 4, "4 stations loaded (header 5 records skipped)");
    check(r.lines == 7 && r.skipped == 3, "7 records after header, 3 skipped (empty / bad id / too few columns)");
    check(r.notUtf8 == 0, "UTF-8: notUtf8 == 0");

    check(st.size() == 4 && st[0].id == "00102" && st[0].name == "망원역 1번출구 앞",
          "first station: id, name trimmed (leading space removed)");
    check(st[0].address == "서울특별시 마포구 월드컵로 72", "address = 상세주소");
    check(st[0].latitude > 37.555 && st[0].latitude < 37.556 && st[0].longitude > 126.910 && st[0].longitude < 126.911,
          "latitude / longitude");
    check(st[0].rackCount == 15, "rackCount: LCD empty + QR 15 = 15");
    check(st[1].rackCount == 12, "rackCount: LCD 12 + QR empty = 12");
    check(st[2].rackCount == 10, "rackCount: LCD 5 + QR 5 = 10");
    check(st[2].address == "강남구 개포동 182-4 (개포동역 4번출구)", "newline inside address -> one field, newline -> space");
    check(st[3].id == "04592", "id padded to 5 digits (\"4592\" -> \"04592\")");
    check(st[3].name == "신길 \"우성\", 4차", "quoted comma and \"\" escape in name");
    check(st[3].rackCount == 10, "row after quoted name parsed correctly");

    // ---- 대여소 트리로 바로 이어지는지 ----
    StationTree tree;
    check(isSortedById(st) && buildStationTree(st, tree) && tree.size() == 4, "result goes straight into buildStationTree");
    check(tree.find("03651") != nullptr && tree.find("03651")->name == "개포동역 4번출구", "tree lookup by id");

    // ---- CRLF (윈도우 줄바꿈, 칸 안 줄바꿈도 CRLF) ----
    writeAll(path, toCrlf(HEADER + BODY));
    DynamicArray<Station> crlf;
    StationParseResult rc;
    loadStations(path, crlf, rc);
    check(rc.loaded == 4 && rc.lines == 7 && rc.skipped == 3, "CRLF: same counts");
    check(crlf.size() == 4 && crlf[2].address == st[2].address && crlf[0].name == st[0].name &&
              crlf[3].name == st[3].name,
          "CRLF: same fields (no stray \r)");

    // ---- BOM 없음 ----
    writeAll(path, HEADER.substr(3) + BODY);
    DynamicArray<Station> nobom;
    StationParseResult rn;
    loadStations(path, nobom, rn);
    check(rn.loaded == 4, "no BOM: same result");

    // ---- 제목 없이 바로 데이터 ----
    writeAll(path, BODY);
    DynamicArray<Station> noHeader;
    StationParseResult rh;
    loadStations(path, noHeader, rh);
    check(rh.loaded == 4 && noHeader[0].id == "00102", "no header: starts at first numeric id");

    // ---- CP949 로 저장한 경우 감지 ----
    // " 망원역" 을 CP949 로: B8 C1 BF F8 BF AA
    writeAll(path, "00102, \xB8\xC1\xBF\xF8\xBF\xAA,\xB8\xB6\xC6\xF7\xB1\xB8,addr,37.5,126.9,2015-09-06,,15,QR,\n");
    DynamicArray<Station> cp;
    StationParseResult rp;
    loadStations(path, cp, rp);
    check(rp.loaded == 1 && rp.notUtf8 == 1, "CP949 file: still loads, notUtf8 counts it");

    // ---- 기존 배열 끝에 추가 ----
    writeAll(path, HEADER + BODY);
    StationParseResult ra;
    loadStations(path, st, ra);
    check(st.size() == 8 && ra.loaded == 4, "appends to existing array; result counts only this file");

    std::remove(path);

    DynamicArray<Station> none;
    StationParseResult rx;
    check(!loadStations("data/__no_such_file__.csv", none, rx), "missing file -> false");

    // ---- isValidUtf8 ----
    check(isValidUtf8("") && isValidUtf8("abc 123") && isValidUtf8("망원역"), "isValidUtf8: ASCII / Korean UTF-8");
    check(!isValidUtf8("\xB8\xC1\xBF\xF8") && !isValidUtf8("\xC0\xDA"), "isValidUtf8: CP949 bytes rejected");
    check(!isValidUtf8("\xEB\xA7") && !isValidUtf8("\xC0\x80"), "isValidUtf8: truncated / overlong rejected");

    std::cout << "RESULT: " << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
