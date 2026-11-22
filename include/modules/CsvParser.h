#ifndef MODULES_CSV_PARSER_H
#define MODULES_CSV_PARSER_H

#include <string>

#include "core/DynamicArray.h"
#include "core/LinkedList.h"
#include "model/Station.h"
#include "model/Trip.h"

// 읽기 결과 통계
struct ParseResult {
    long long lines;     // 헤더 제외 전체 줄 수
    long long loaded;    // Trip 으로 변환해 저장한 수
    long long skipped;   // 빈 대여소번호, 형식 오류 등으로 건너뛴 수

    ParseResult() : lines(0), loaded(0), skipped(0) {}
};

// 대여이력 CSV 를 읽어 trips 끝에 추가한다.
// - 원본(CP949)과 sample.csv(UTF-8), UTF-8 BOM, CRLF 줄바꿈 모두 같은 결과 (tests/test_csv_parser.cpp)
//   Trip 에 담는 컬럼(번호·일시·숫자)은 모두 영문/숫자라 인코딩과 무관하다.
//   CP949 둘째 바이트(0x41~0xFE)와 UTF-8 이어지는 바이트(0x80~0xBF)는 ',' '"' 와 겹치지 않아
//   한글 대여소명이 있어도 칸 나누기가 깨지지 않는다. BOM 은 헤더 줄에만 있어 건너뛰어진다.
// - 파일을 열지 못하면 false
bool loadTrips(const std::string& path, LinkedList<Trip>& trips, ParseResult& result);

// "2026-06-01 00:01:33" → 20260601000133. 형식이 틀리면 -1
long long parseDateTime(const std::string& text);

// 대여소번호를 5자리 문자열로 맞춘다. " 102" → "00102", "00393" → "00393"
// 숫자가 아니거나 비어 있으면 "" (대여소 정보 xlsx 를 읽을 때도 사용)
std::string normalizeStationId(const std::string& text);

// ---------------------------------------------------------------------------
// 대여소 정보 (단계 1-1)
// ---------------------------------------------------------------------------

// 대여소 정보 읽기 결과 통계
struct StationParseResult {
    long long lines   = 0;  // 제목 다음부터 읽은 줄(레코드) 수 (빈 줄 제외)
    long long loaded  = 0;  // Station 으로 변환해 저장한 수
    long long skipped = 0;  // 대여소번호가 숫자가 아니거나 비어 있어 건너뛴 수
    long long notUtf8 = 0;  // 대여소명·주소가 UTF-8 이 아닌 줄 수 (0 이 아니면 CP949 로 저장된 파일)
};

// 대여소 정보 CSV 를 읽어 stations 끝에 추가한다.
// - 원본은 xlsx → 엑셀에서 "CSV UTF-8(쉼표로 분리)" 로 저장해서 사용.
//   일반 "CSV(쉼표로 분리)" 로 저장하면 CP949 라 대여소명이 깨진다 → result.notUtf8 로 확인
// - 맨 위 제목 줄(엑셀 1~5행)은 대여소번호 칸이 숫자인 첫 줄이 나올 때까지 건너뛴다.
//   제목 칸 안의 줄바꿈("대여소\n번호")과 주소 칸 안의 줄바꿈도 한 칸으로 읽는다 (줄바꿈 → 공백).
// - 칸: 대여소번호, 대여소명, 자치구, 상세주소, 위도, 경도, 설치시기, LCD 거치대수, QR 거치대수, 운영방식
//   address = 상세주소 (상세주소에 이미 자치구가 들어 있는 경우가 많아 그대로 사용)
//   rackCount = LCD 거치대수 + QR 거치대수 (빈 칸은 0)
// - 파일 순서(대여소번호 오름차순) 그대로 넣는다 → buildStationTree 에 바로 넘길 수 있음
// - 파일을 열지 못하면 false
bool loadStations(const std::string& path, DynamicArray<Station>& stations, StationParseResult& result);

// 문자열이 올바른 UTF-8 인지 (ASCII 만 있으면 true)
bool isValidUtf8(const std::string& text);

#endif
