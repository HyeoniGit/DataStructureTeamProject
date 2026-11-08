#ifndef MODULES_BIKE_INDEX_H
#define MODULES_BIKE_INDEX_H

#include <ostream>
#include <string>

#include "core/BST.h"
#include "core/LinkedList.h"
#include "model/Trip.h"
#include "modules/StationIndex.h"

// 자전거 한 대의 이동 기록 (자전거 번호 트리의 노드 값)
// - trips 에는 대여 기록을 복사하지 않고 주소만 담는다. 원본은 대여이력 연결 리스트에 한 번만 있다.
// - 대여이력이 반납일시 순이고 자전거 한 대가 동시에 두 번 빌려질 수 없으므로,
//   끝에 붙이기만 해도 자전거별로 시간순이 된다.
struct BikeRoute {
    std::string bikeId;
    LinkedList<const Trip*> trips;
};

// 자전거 번호 트리 (단계 2-3, 3-3)
// - 키: 자전거번호 ("SPB-58380"), 값: BikeRoute
// - 자전거 1대 = 노드 1개, 이동 1번 = 노드 안 리스트 원소 1개
typedef BST<std::string, BikeRoute> BikeTree;

// 대여이력을 처음부터 읽어 자전거 번호 트리를 만든다.
// - 처음 보는 번호면 노드를 새로 만들고, 있으면 그 노드의 리스트 끝에 기록 주소를 추가
// - 자전거번호가 빈 기록은 건너뛴다
// - trips 는 트리를 쓰는 동안 살아 있어야 한다 (주소를 가리키므로)
// 반환: 트리에 넣은 기록 수
// 복잡도: 기록 m건, 자전거 n대일 때 평균 O(m log n)
long long buildBikeTree(const LinkedList<Trip>& trips, BikeTree& tree);

// 자전거 이동 경로 출력 (메뉴: 자전거 검색)
// - 대여소 이름은 대여소 번호 트리에서 찾고, 없는 번호면 번호를 그대로 보여 준다
// - 없는 자전거면 false
bool printBikeRoute(const BikeTree& bikes, const StationTree& stations,
                    const std::string& bikeId, std::ostream& out);

// 20260601081233 → "06-01 08:12"
std::string formatDateTime(long long dateTime);

#endif
