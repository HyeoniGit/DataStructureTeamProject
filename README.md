# 따릉이 대여소 이동 연결망 분석

서울시 공공자전거 따릉이 **대여소 정보**와 **대여이력** 데이터로 대여소 간 이동을 방향 가중 그래프로 만들고,
수업에서 배운 자료구조(동적 배열, 연결 리스트, 템플릿, 큐, BST, 정렬·이분 탐색, 그래프)를 C++로 직접 구현해 분석하는 프로젝트입니다.

- 노드: 대여소 / 간선: 대여 대여소 → 반납 대여소 / 가중치: 이동 횟수, 총 이용시간

## 실행 방법

### 1) make (Mac / Linux, 권장)

```bash
make run     # 빌드 후 data/sample.csv 로 실행
make clean   # 빌드 결과 삭제
```

### 2) Visual Studio (Windows)

1. 빈 프로젝트 생성
2. `src/` 와 `src/modules/` 의 `.cpp` 파일, `include/` 의 `.h` 파일을 프로젝트에 추가
3. 프로젝트 속성 → C/C++ → 일반 → **추가 포함 디렉터리**에 `include` 폴더 추가
4. 프로젝트 속성 → 디버깅 → **명령 인수**에 `data/sample.csv` 입력
5. 실행 (Ctrl + F5)

> 원본 CSV는 용량이 커서 저장소에 포함하지 않았습니다. `data/sample.csv` 로 바로 실행할 수 있습니다.

g++ 한 줄 빌드: `g++ -std=c++17 -Iinclude src/*.cpp src/modules/*.cpp -o ddareungi && ./ddareungi data/sample.csv`

CMake를 쓰는 경우: `cmake -B build && cmake --build build`

## 사용 데이터

- [서울시 공공자전거 따릉이 대여소 정보](https://data.seoul.go.kr/dataList/OA-13252/F/1/datasetView.do): 대여소번호, 대여소명, 위치, 거치대 수 (`.xlsx` → CSV(UTF-8)로 변환해서 사용)
- [서울시 공공자전거 따릉이 대여이력 정보](https://data.seoul.go.kr/dataList/OA-15182/F/1/datasetView.do): 월 단위 CSV
- 원본 CSV는 용량이 커서 저장소에 올리지 않습니다. 내려받아 `data/` 폴더에 넣으세요.
- `data/sample.csv`: 테스트용 20줄

## 폴더 구조

```
include/
  core/       템플릿 자료구조 (DynamicArray, LinkedList, Queue, BST)
  model/      공통 구조체 (Trip, Station)
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
| 동적 배열 | `core/DynamicArray.h` | 정렬용 포인터 배열 (대여 기록·대여소 노드 주소 복사) |
| 연결 리스트 | `core/LinkedList.h` | 대여이력 저장, 그래프 인접 리스트, 자전거별 이용 기록(시간순) |
| 템플릿 | `core/*.h` | 모든 자료구조를 타입 무관하게 구현 |
| 큐 | `core/Queue.h`, `modules/Traversal` | BFS: 대여소 간 최소 간선 경로 |
| BST | `core/BST.h`, `modules/StationIndex` | 대여소번호·자전거번호 검색, 자전거별 이동 경로 |
| 정렬·이분 탐색 | `modules/Analysis` | 병합 정렬(대여일시 / 대여소 이용 횟수), Top-N, 대여일시 기간 검색 |
| 방향 가중 그래프 | `modules/Graph` | 대여 → 반납 이동 연결망, 진입·진출 차수, 편도 경로 |

## 핵심 로직

1. 대여소 정보 CSV 파싱 → 대여소번호 기준 BST 삽입 (중복은 건너뜀)
2. 대여이력 CSV 파싱 → 필요한 컬럼만 `Trip` 구조체로 → 연결 리스트 끝에 추가
3. 대여 기록마다 대여소 BST에서 대여·반납 노드 검색 (없으면 건너뜀) → 그래프에 "대여 → 반납" 간선 추가 (있으면 이동 횟수·이용시간 누적) → 진입·진출 차수, 편도 경로(A→B vs B→A) 판별
4. 자전거번호 BST 구성, 노드마다 이용 기록 연결 리스트(시간순)
5. 노드 주소를 포인터 배열에 복사 → 병합 정렬 (대여 기록: 대여일시 / 대여소: 이용 횟수)
6. 메뉴 입력에 따라 BST 검색 / 자전거별 이동 경로 / 이분 탐색 기간 검색 / Top-N / BFS 최소 간선 경로 결과 출력

> 정렬은 병합 정렬 하나로 통일 (항상 O(n log n), 안정 정렬. 대여일시가 거의 정렬된 상태라 퀵 정렬은 최악에 가까울 수 있음)
>
> BST는 정렬된 순서로 넣으면 한쪽으로 쏠림(최악 O(n)) → 선택 기능: 높이 측정 후 중위 순회 배열로 균형 재구축, 전후 높이 비교

---

## 팀 작업 규칙 (최종 제출 전 이 섹션 삭제)

**담당 파일**

| 담당 | 파일 |
|---|---|
| 이은주 | `core/DynamicArray.h`, `core/LinkedList.h`, `core/Queue.h`, `modules/CsvParser.*`, `modules/Traversal.*` |
| 주승현 | `core/BST.h`, `modules/StationIndex.*`, `README.md` |
| 박민수 | `modules/Graph.*` |
| 추송주 | `modules/Analysis.*`, `main.cpp` |

- 각자 코드에 `TODO(담당)` 표시된 부분부터 채우면 됩니다.
- `include/model/` 의 공통 구조체와 모듈 간 함수 형태(예: `Graph::neighbors`)를 바꿀 땐 팀방에 먼저 공유.
- **STL 컨테이너(vector, map 등) 대신 직접 구현한 자료구조 사용.** (`std::string`, 입출력은 사용)
- 커밋은 반드시 **본인 GitHub 계정**으로 (기여도 근거).
- 작업은 `본인이름/기능` 브랜치에서 하고 main 에 PR 로 합치기 권장.
- 대여소번호는 앞자리 0 때문에 **문자열**로 다룸 ("00393").
- 대여소는 대여소 BST 노드에 저장. 노드는 재할당되지 않으므로 노드 포인터로 참조.
