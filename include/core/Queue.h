#ifndef CORE_QUEUE_H
#define CORE_QUEUE_H

#include <cstddef>

#include "core/LinkedList.h"

// 템플릿 큐 (선입선출)
// - 연결 리스트로 구현: 뒤에 넣고(pushBack) 앞에서 꺼냄(popFront), 둘 다 O(1)
// - BFS 에서 방문할 대여소를 순서대로 꺼낼 때 사용
//
// 사용 예:
//   Queue<std::string> q;
//   q.enqueue("00102");
//   while (!q.empty()) {
//       std::string cur = q.front();
//       q.dequeue();
//       ...
//   }
template <typename T>
class Queue {
public:
    // 뒤에 추가 O(1)
    void enqueue(const T& value) { list_.pushBack(value); }

    // 맨 앞 제거 O(1). 비어 있으면 false
    bool dequeue() { return list_.popFront(); }

    // 맨 앞 원소. 비어 있을 때 호출하면 안 됨 (empty() 먼저 확인)
    T& front() { return list_.head()->data; }
    const T& front() const { return list_.head()->data; }

    std::size_t size() const { return list_.size(); }
    bool empty() const { return list_.empty(); }
    void clear() { list_.clear(); }

private:
    LinkedList<T> list_;
};

#endif
