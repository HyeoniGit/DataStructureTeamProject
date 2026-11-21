#ifndef MODULES_CSV_PARSER_H
#define MODULES_CSV_PARSER_H

#include <string>

#include "core/LinkedList.h"
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

#endif
