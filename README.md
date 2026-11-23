# 따릉이 대여소 이동 연결망 분석

서울시 공공자전거 따릉이 **대여소 정보**와 **대여이력** 데이터로 대여소 간 이동을 방향 가중 그래프로 만들고,
수업에서 배운 자료구조(동적 배열, 연결 리스트, 템플릿, 큐, BST, 정렬·이분 탐색, 그래프)를 C++로 직접 구현해 분석하는 프로젝트입니다.

- 노드: 대여소 / 간선: 대여 대여소 → 반납 대여소 / 가중치: 이동 횟수, 총 이용시간

## 실행 방법

### 1) make (Mac / Linux, 권장)

```bash
make run     # 빌드 후 샘플 데이터(data/sample_stations.csv, data/sample.csv)로 실행
make test    # tests/ 의 테스트 실행
make clean   # 빌드 결과 삭제
```

### 2) Visual Studio (Windows)

1. 빈 프로젝트 생성
2. `src/` 와 `src/modules/` 의 `.cpp` 파일, `include/` 의 `.h` 파일을 프로젝트에 추가
3. 프로젝트 속성 → C/C++ → 일반 → **추가 포함 디렉터리**에 `include` 폴더 추가
4. 프로젝트 속성 → C/C++ → 명령줄 → **추가 옵션**에 `/utf-8` 입력 (소스가 UTF-8이라 없으면 한글 문자열에서 빌드 오류)
5. 프로젝트 속성 → 디버깅 → **작업 디렉터리**를 저장소 최상위 폴더로 지정. **명령 인수**를 비워 두면 샘플 데이터로 실행 (실제 데이터는 아래 "실행 인수" 참고)
6. 실행 (Ctrl + F5)

> Windows 콘솔은 기본이 CP949라 UTF-8 한글이 깨지므로 `main` 시작에서 `SetConsoleOutputCP(CP_UTF8)` 를 호출합니다.

g++ 한 줄 빌드: `g++ -std=c++17 -Iinclude src/*.cpp src/modules/*.cpp -o ddareungi && ./ddareungi`

### 실행 인수와 메뉴

```bash
./ddareungi [대여소 정보 CSV] [대여이력 CSV]
./ddareungi       # 인수 없음 → 샘플: data/sample_stations.csv, data/sample.csv
./ddareungi "data/공공자전거 대여소 정보(26.6월 기준).csv" "data/서울특별시 공공자전거 대여이력 정보_2606.csv"
```

- 시작할 때 대여소 트리 → 대여이력 리스트 → 그래프 → 자전거 트리 → 정렬용 배열을 만들고 메뉴를 띄웁니다.
- 메뉴 번호(1~7)를 고르고 값을 입력하면 결과를 출력한 뒤 다시 메뉴로 돌아옵니다. **`q` 를 입력하면 종료**하고, 값을 묻는 중에 빈 Enter 를 누르면 메뉴로 돌아갑니다.

> 원본 CSV는 용량이 커서 저장소에 포함하지 않았습니다. 인수 없이 실행하면 샘플 데이터로 바로 실행됩니다.

CMake를 쓰는 경우: `cmake -B build && cmake --build build`

## 사용 데이터

- [서울시 공공자전거 따릉이 대여소 정보](https://data.seoul.go.kr/dataList/OA-13252/F/1/datasetView.do): 대여소번호, 대여소명, 위치, 거치대 수 (`.xlsx` → CSV(UTF-8)로 변환해서 사용)
- [서울시 공공자전거 따릉이 대여이력 정보](https://data.seoul.go.kr/dataList/OA-15182/F/1/datasetView.do): 월 단위 CSV
- 원본 CSV는 용량이 커서 저장소에 올리지 않습니다. 내려받아 `data/` 폴더에 넣으세요.
- `data/sample.csv`: 대여이력 샘플 (헤더 + 19건)
- `data/sample_stations.csv`: 대여소 정보 샘플 (sample.csv 에 나오는 대여소 30곳, 원본과 같은 형식)

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
| BST | `core/BST.h`, `modules/StationIndex`, `modules/BikeIndex` | 대여소 번호 트리·자전거 번호 트리, 검색, 자전거별 이동 경로 |
| 정렬·이분 탐색 | `modules/Analysis` | 병합 정렬(대여일시 / 대여소 이용 횟수), Top-N, 대여일시 기간 검색 |
| 방향 가중 그래프 | `modules/Graph` | 대여 → 반납 이동 연결망, 진입·진출 차수, 편도 경로 |

## 핵심 로직

1. 대여소 정보 CSV 파싱 → 대여소번호 기준 BST 삽입 (중복은 건너뜀). 대여소 정보는 번호 순으로 정렬돼 있어 배열의 가운데부터 재귀로 넣어 균형 유지
2. 대여이력 CSV 파싱 → 필요한 컬럼만 `Trip` 구조체로 → 연결 리스트 끝에 추가
3. 대여 기록마다 대여소 BST에서 대여·반납 노드 검색 (없으면 건너뜀) → 그래프에 "대여 → 반납" 간선 추가 (있으면 이동 횟수·이용시간 누적) → 진입·진출 차수, 편도 경로(A→B vs B→A) 판별
4. 자전거번호 BST 구성, 노드마다 이용 기록 연결 리스트(시간순)
5. 노드 주소를 포인터 배열에 복사 → 병합 정렬 (대여 기록: 대여일시 / 대여소: 이용 횟수)
6. 메뉴 입력에 따라 BST 검색 / 자전거별 이동 경로 / 이분 탐색 기간 검색 / Top-N / BFS 최소 간선 경로 결과 출력

> 정렬은 병합 정렬 하나로 통일 (항상 O(n log n), 안정 정렬. 대여일시가 거의 정렬된 상태라 퀵 정렬은 최악에 가까울 수 있음)
>
> BST는 정렬된 순서로 넣으면 한쪽으로 쏠림(최악 O(n)) → 대여소 트리는 정렬된 배열의 가운데부터 넣음 (대여소 2,789개 기준 높이 2,789 → 12). 자전거번호는 대여이력에서 섞인 순서로 나와 그대로 넣음

---

## 팀 작업 규칙 (최종 제출 전 이 섹션 삭제)

**담당 파일**

| 담당 | 파일 |
|---|---|
| 이은주 | `core/DynamicArray.h`, `core/LinkedList.h`, `core/Queue.h`, `modules/CsvParser.*`, `modules/Traversal.*` |
| 주승현 | `core/BST.h`, `modules/StationIndex.*`, `modules/BikeIndex.*`, `README.md` |
| 박민수 | `modules/Graph.*` |
| 추송주 | `modules/Analysis.*`, `main.cpp` |

- 각자 코드에 `TODO(담당)` 표시된 부분부터 채우면 됩니다.
- `include/model/` 의 공통 구조체와 모듈 간 함수 형태(예: `Graph::neighbors`)를 바꿀 땐 팀방에 먼저 공유.
- **STL 컨테이너(vector, map 등) 대신 직접 구현한 자료구조 사용.** (`std::string`, 입출력은 사용)
- 커밋은 반드시 **본인 GitHub 계정**으로 (기여도 근거).
- 작업은 `본인이름/기능` 브랜치에서 하고 main 에 PR 로 합치기 권장.
- 대여소번호는 앞자리 0 때문에 **문자열**로 다룸 ("00393").
- 대여소는 대여소 BST 노드에 저장. 노드는 재할당되지 않으므로 노드 포인터로 참조.
