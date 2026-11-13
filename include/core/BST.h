#ifndef CORE_BST_H
#define CORE_BST_H

#include <cstddef>

// 템플릿 이진 탐색 트리 (BST)
// - 키 K 로 비교: 작으면 왼쪽, 크면 오른쪽 (K 에 operator< 필요)
// - 값 V 는 노드 안에 저장된다. 노드는 한 번 만들어지면 위치가 바뀌지 않으므로
//   find/insert 가 돌려준 V* 를 다른 곳에서 계속 가리켜도 안전하다.
// - 같은 키는 하나만 저장한다. 이미 있는 키를 insert 하면 새로 만들지 않고 기존 값의 주소를 돌려준다.
// - 삭제는 없다. (데이터를 한 번 읽어 조회만 하는 프로그램)
//
// 사용처
// - 대여소 번호 트리: BST<std::string, Station>     (단계 1-1)
// - 자전거 번호 트리: BST<std::string, BikeRoute>   (단계 2-3)
//
// 복잡도: 삽입·검색 평균 O(log n), 최악 O(n) (한쪽으로 쏠린 경우)
//   → 정렬된 데이터는 StationIndex 의 buildStationTree 처럼 가운데부터 넣어 균형을 맞춘다.
template <typename K, typename V>
class BST {
public:
    BST() : root_(nullptr), size_(0) {}
    ~BST() { destroy(root_); }

    // 노드 소유권이 있으므로 복사 금지 (V* 주소가 꼬이는 것을 막기 위함)
    BST(const BST&) = delete;
    BST& operator=(const BST&) = delete;

    // 삽입. 새 노드 또는 이미 있던 노드의 값 주소를 돌려준다.
    V* insert(const K& key, const V& value) {
        Node** link = &root_;
        while (*link != nullptr) {
            Node* cur = *link;
            if (key < cur->key) {
                link = &cur->left;
            } else if (cur->key < key) {
                link = &cur->right;
            } else {
                return &cur->value;          // 이미 있는 키: 그대로 둔다
            }
        }
        *link = new Node(key, value);
        ++size_;
        return &(*link)->value;
    }

    // 검색. 없으면 nullptr
    V* find(const K& key) {
        Node* cur = root_;
        while (cur != nullptr) {
            if (key < cur->key) cur = cur->left;
            else if (cur->key < key) cur = cur->right;
            else return &cur->value;
        }
        return nullptr;
    }

    const V* find(const K& key) const {
        return const_cast<BST*>(this)->find(key);
    }

    bool contains(const K& key) const { return find(key) != nullptr; }

    // key 를 찾을 때까지 비교한 횟수. 없으면 -1 (보고서용 비교 횟수 측정)
    int searchSteps(const K& key) const {
        int steps = 0;
        const Node* cur = root_;
        while (cur != nullptr) {
            ++steps;
            if (key < cur->key) cur = cur->left;
            else if (cur->key < key) cur = cur->right;
            else return steps;
        }
        return -1;
    }

    // 중위 순회 (재귀) — 키 오름차순으로 visit(key, value) 호출
    template <typename F>
    void inorder(F visit) { inorder(root_, visit); }

    template <typename F>
    void inorder(F visit) const { inorderConst(root_, visit); }

    // 높이: 빈 트리 0, 노드 1개 1
    int height() const { return height(root_); }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    void clear() {
        destroy(root_);
        root_ = nullptr;
        size_ = 0;
    }

private:
    struct Node {
        K key;
        V value;
        Node* left;
        Node* right;
        Node(const K& k, const V& v) : key(k), value(v), left(nullptr), right(nullptr) {}
    };

    template <typename F>
    static void inorder(Node* node, F& visit) {
        if (node == nullptr) return;
        inorder(node->left, visit);
        visit(node->key, node->value);
        inorder(node->right, visit);
    }

    template <typename F>
    static void inorderConst(const Node* node, F& visit) {
        if (node == nullptr) return;
        inorderConst(node->left, visit);
        visit(node->key, node->value);
        inorderConst(node->right, visit);
    }

    static int height(const Node* node) {
        if (node == nullptr) return 0;
        int l = height(node->left);
        int r = height(node->right);
        return 1 + (l > r ? l : r);
    }

    static void destroy(Node* node) {
        if (node == nullptr) return;
        destroy(node->left);
        destroy(node->right);
        delete node;
    }

    Node* root_;
    std::size_t size_;
};

#endif
