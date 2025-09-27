#pragma once
#include <utility>

template<typename T>
class SmartPtr {
private:
    T* ptr;
    std::size_t* ref_count;

    void release() {
        if (ref_count) {
            (*ref_count)--;
            if (*ref_count == 0) {
                delete ptr;
                delete ref_count;
            }
        }
    }

public:
    // Конструкторы
    SmartPtr() : ptr(nullptr), ref_count(nullptr) {}

    explicit SmartPtr(T* p) : ptr(p), ref_count(new std::size_t(1)) {}

    SmartPtr(const SmartPtr& other) : ptr(other.ptr), ref_count(other.ref_count) {
        if (ref_count) (*ref_count)++;
    }

    SmartPtr& operator=(const SmartPtr& other) {
        if (this != &other) {
            release();
            ptr = other.ptr;
            ref_count = other.ref_count;
            if (ref_count) (*ref_count)++;
        }
        return *this;
    }

    ~SmartPtr() {
        release();
    }

    // Разыменование
    T& operator*() const { return *ptr; }
    T* operator->() const { return ptr; }
    T* get() const { return ptr; }

    // Методы
    void reset(T* p = nullptr) {
        release();
        if (p) {
            ptr = p;
            ref_count = new std::size_t(1);
        } else {
            ptr = nullptr;
            ref_count = nullptr;
        }
    }

    void swap(SmartPtr& other) {
        std::swap(ptr, other.ptr);
        std::swap(ref_count, other.ref_count);
    }

    std::size_t use_count() const {
        return ref_count ? *ref_count : 0;
    }

    // Сравнение
    bool operator==(const SmartPtr& other) const { return ptr == other.ptr; }
    bool operator!=(const SmartPtr& other) const { return ptr != other.ptr; }
    bool operator<(const SmartPtr& other) const { return ptr < other.ptr; }
    bool operator>(const SmartPtr& other) const { return ptr > other.ptr; }
    bool operator<=(const SmartPtr& other) const { return ptr <= other.ptr; }
    bool operator>=(const SmartPtr& other) const { return ptr >= other.ptr; }
};

// Фабричная функция
template<typename T, typename... Args>
SmartPtr<T> make_smart_ptr(Args&&... args) {
    return SmartPtr<T>(new T(std::forward<Args>(args)...));
}
