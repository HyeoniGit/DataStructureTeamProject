#ifndef CORE_DYNAMIC_ARRAY_H
#define CORE_DYNAMIC_ARRAY_H

#include <cstddef>

// 템플릿 동적 배열
// - 끝에 추가: 평균 O(1) (공간이 모자라면 2배로 늘려 복사)
// - 인덱스 접근 O(1): arr[i]
// - 정렬용 포인터 배열로 사용 (예: DynamicArray<Trip*>, DynamicArray<Station*>)
//   → 개수를 알면 reserve(n) 로 미리 공간을 잡아 재할당을 줄인다.
// - 공간을 늘릴 때 원소 위치가 바뀌므로, 원소의 주소(&arr[i])를 따로 보관하지 말 것.
// - 복사하면 원소까지 새로 만든다(깊은 복사).
template <typename T>
class DynamicArray {
public:
    DynamicArray() : data_(nullptr), size_(0), capacity_(0) {}

    // 원소 n 개를 value 로 채워서 만든다 (병합 정렬 임시 배열 등)
    DynamicArray(std::size_t n, const T& value) : data_(nullptr), size_(0), capacity_(0) {
        reserve(n);
        for (std::size_t i = 0; i < n; ++i) data_[i] = value;
        size_ = n;
    }

    DynamicArray(const DynamicArray& other) : data_(nullptr), size_(0), capacity_(0) {
        copyFrom(other);
    }

    DynamicArray& operator=(const DynamicArray& other) {
        if (this != &other) {
            delete[] data_;
            data_ = nullptr;
            size_ = capacity_ = 0;
            copyFrom(other);
        }
        return *this;
    }

    ~DynamicArray() { delete[] data_; }

    // 끝에 추가. 공간이 모자라면 2배로 늘림
    void pushBack(const T& value) {
        if (size_ == capacity_) reserve(capacity_ == 0 ? 16 : capacity_ * 2);
        data_[size_++] = value;
    }

    // 맨 끝 원소 제거. 비어 있으면 false
    bool popBack() {
        if (size_ == 0) return false;
        --size_;
        return true;
    }

    // 최소 n 개를 담을 공간 확보 (이미 충분하면 아무것도 안 함)
    void reserve(std::size_t n) {
        if (n <= capacity_) return;
        T* bigger = new T[n];
        for (std::size_t i = 0; i < size_; ++i) bigger[i] = data_[i];
        delete[] data_;
        data_ = bigger;
        capacity_ = n;
    }

    // 원소만 비움 (공간은 유지)
    void clear() { size_ = 0; }

    // 범위 검사 없음: 0 <= i < size() 에서만 사용
    T& operator[](std::size_t i) { return data_[i]; }
    const T& operator[](std::size_t i) const { return data_[i]; }

    std::size_t size() const { return size_; }
    std::size_t capacity() const { return capacity_; }
    bool empty() const { return size_ == 0; }

private:
    void copyFrom(const DynamicArray& other) {
        reserve(other.size_);
        for (std::size_t i = 0; i < other.size_; ++i) data_[i] = other.data_[i];
        size_ = other.size_;
    }

    T* data_;
    std::size_t size_;
    std::size_t capacity_;
};

#endif
