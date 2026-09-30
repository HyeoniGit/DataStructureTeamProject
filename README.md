# 따릉이 대여소 이동 연결망 분석

서울시 공공자전거 따릉이 **대여이력** 데이터로 대여소 간 이동을 방향 가중 그래프로 만들고,
수업에서 배운 자료구조(동적 배열, 연결 리스트, 템플릿, 큐, BST, 정렬·이분 탐색, 그래프)를 C++로 직접 구현해 분석하는 프로젝트입니다.

- 노드: 대여소 / 간선: 대여 대여소 → 반납 대여소 / 가중치: 이동 횟수, 총 이용시간

## 실행 방법

### 1) g++ (Linux / Mac / Git Bash)

```bash
g++ -std=c++17 -Iinclude src/*.cpp src/modules/*.cpp -o ddareungi
./ddareungi data/sample.csv            # 샘플 데이터로 실행
./ddareungi data/<원본파일>.csv 100000  # 원본 중 앞 10만 건만
```

### 2) Visual Studio (Windows)

1. 빈 프로젝트 생성
2. `src/` 와 `src/modules/` 의 `.cpp` 파일, `include/` 의 `.h` 파일을 프로젝트에 추가
3. 프로젝트 속성 → C/C++ → 일반 → **추가 포함 디렉터리**에 `include` 폴더 추가
4. 프로젝트 속성 → 디버깅 → **명령 인수**에 `data/sample.csv` 입력
5. 실행 (Ctrl + F5)

> 원본 CSV는 용량이 커서 저장소에 포함하지 않았습니다. `data/sample.csv` 로 바로 실행할 수 있습니다.

CMake를 쓰는 경우: `cmake -B build && cmake --build build`

## 사용 데이터

- [서울시 공공자전거 따릉이 대여이력 정보](https://data.seoul.go.kr/dataList/OA-15182/F/1/datasetView.do)
- 원본 CSV는 용량이 커서 저장소에 올리지 않습니다. 내려받아 `data/` 폴더에 넣으세요.
- `data/sample.csv`: 테스트용 20줄

## 폴더 구조

```
include/
  core/       템플릿 자료구조 (DynamicArray, LinkedList, Queue, BST)
  model/      공통 구조체 (Trip, Station, DataStore)
  modules/    기능별 모듈 헤더
src/
  main.cpp    메뉴 (입력 → 결과 출력)
  modules/    기능별 구현
data/         데이터 (sample.csv만 커밋)
docs/         제안서·보고서 자료
```

## 자료구조 활용

| 자료구조 | 파일 | 적용 |
|---|---|---|
| 동적 배열 | `core/DynamicArray.h` | 대여이력·대여소 목록 저장 |
| 연결 리스트 | `core/LinkedList.h` | 그래프 인접 리스트, 자전거별 이동 경로 |
| 템플릿 | `core/*.h` | 모든 자료구조를 타입 무관하게 구현 |
| 큐 | `core/Queue.h`, `modules/Traversal` | BFS: N번 이동 내 도달 가능 대여소 |
| BST | `core/BST.h`, `modules/StationIndex` | 대여소번호·자전거번호 검색 |
| 정렬·이분 탐색 | `modules/Analysis` | 이용량 Top-N, 대여일시 기간 검색 |
| 방향 가중 그래프 | `modules/Graph` | 대여 → 반납 이동 연결망 |

## 핵심 로직

1. CSV 파싱 → 정제 → `trips` 동적 배열 저장
2. 대여이력을 돌며 대여소번호를 BST로 색인, 대여·반납 건수 누적
3. 같은 순회로 그래프에 "대여 → 반납" 간선 추가 (있으면 횟수 +1)
4. 메뉴 입력에 따라 BST 검색 / BFS / 정렬·이분 탐색 결과 출력

---

## 팀 작업 규칙 (최종 제출 전 이 섹션 삭제)

**담당 파일**

| 담당 | 파일 |
|---|---|
| A | `core/DynamicArray.h`, `core/LinkedList.h`, `core/Queue.h`, `modules/CsvParser.*`, `modules/Traversal.*` |
| B | `core/BST.h`, `modules/StationIndex.*` |
| C | `modules/Graph.*` |
| D | `modules/Analysis.*`, `main.cpp`, `README.md` |

- 각자 코드에 `TODO(담당)` 표시된 부분부터 채우면 됩니다.
- `include/model/` 의 공통 구조체와 모듈 간 함수 형태(예: `Graph::neighbors`)를 바꿀 땐 팀방에 먼저 공유.
- **STL 컨테이너(vector, map 등) 대신 직접 구현한 자료구조 사용.** (`std::string`, 입출력은 사용)
- 커밋은 반드시 **본인 GitHub 계정**으로 (기여도 근거).
- 작업은 `본인이름/기능` 브랜치에서 하고 main 에 PR 로 합치기 권장.
- 대여소번호는 앞자리 0 때문에 **문자열**로 다룸 ("00393").
- `stations` 배열은 커지면 재할당되므로 `Station*` 대신 **인덱스(int)** 로 참조.
