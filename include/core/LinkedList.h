#ifndef CORE_LINKED_LIST_H
#define CORE_LINKED_LIST_H

#include <cstddef>

// 템플릿 단일 연결 리스트
// - 끝에 추가 O(1) (tail 포인터 유지)
// - 순회: for (auto* n = list.head(); n != nullptr; n = n->next) { n->data ... }
// - 복사하면 노드까지 새로 만든다(깊은 복사).
//   Station 안의 인접 리스트가 배열 재할당 때 복사돼도 안전하게 하기 위함.
template <typename T>
class LinkedList {
public:
    struct Node {
        T data;
        Node* next;
        explicit Node(const T& value) : data(value), next(nullptr) {}
    };

    LinkedList() : head_(nullptr), tail_(nullptr), size_(0) {}

    LinkedList(const LinkedList& other) : head_(nullptr), tail_(nullptr), size_(0) {
        copyFrom(other);
    }

    LinkedList& operator=(const LinkedList& other) {
        if (this != &other) {
            clear();
            copyFrom(other);
        }
        return *this;
    }

    ~LinkedList() { clear(); }

    // 끝에 추가 O(1)
    void pushBack(const T& value) {
        Node* node = new Node(value);
        if (tail_ == nullptr) {
            head_ = tail_ = node;
        } else {
            tail_->next = node;
            tail_ = node;
        }
        ++size_;
    }

    // 앞에 추가 O(1)
    void pushFront(const T& value) {
        Node* node = new Node(value);
        node->next = head_;
        head_ = node;
        if (tail_ == nullptr) tail_ = node;
        ++size_;
    }

    // 맨 앞 노드 제거 (큐의 dequeue 에서 사용). 비어 있으면 false
    bool popFront() {
        if (head_ == nullptr) return false;
        Node* old = head_;
        head_ = head_->next;
        if (head_ == nullptr) tail_ = nullptr;
        delete old;
        --size_;
        return true;
    }

    void clear() {
        while (head_ != nullptr) {
            Node* next = head_->next;
            delete head_;
            head_ = next;
        }
        tail_ = nullptr;
        size_ = 0;
    }

    Node* head() { return head_; }
    const Node* head() const { return head_; }
    Node* tail() { return tail_; }
    const Node* tail() const { return tail_; }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

private:
    void copyFrom(const LinkedList& other) {
        for (const Node* n = other.head_; n != nullptr; n = n->next) {
            pushBack(n->data);
        }
    }

    Node* head_;
    Node* tail_;
    std::size_t size_;
};

#endif
