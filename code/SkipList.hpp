#ifndef Skiplist_HPP
#define Skiplist_HPP

#include <random>


template <typename T>
class SkipList {
public:
    SkipList(int max_layer, float p);
    ~SkipList();
    virtual auto Add(T key) -> bool = 0;
    virtual auto Remove(T key) -> bool = 0;
    virtual auto Contains(T key) -> bool = 0;
    virtual void Print() = 0;

protected:
    int max_layer_;
    float P_;
    int RandomLayer();
};

// implementation
template <typename T>
SkipList<T>::SkipList(int max_layer, float p) {
    max_layer_ = max_layer;
    P_ = p;
}

template <typename T>
SkipList<T>::~SkipList() {
}

template <typename T>
int SkipList<T>::RandomLayer() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 1);
    int layer = 1;
    while (dis(gen) < P_ && layer < max_layer_ - 1) {
        ++layer;
    }
    return layer;
}

#endif // Skiplist_HPP