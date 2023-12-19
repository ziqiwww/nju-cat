#ifndef NaiveSkipList_HPP
#define NaiveSkipList_HPP

#include "SkipList.hpp"
#include <limits>
#include <random>
#include <iostream>

template <typename T>
class SkipListNode {
public:
    T key_;
    int top_layer_;
    SkipListNode<T> **next_;
};

template <typename T>
class NaiveSkipList : public SkipList<T> {
public:
    NaiveSkipList(int max_layer, float p);
    ~NaiveSkipList();
    
    auto Add(T key) -> bool;
    auto Remove(T key) -> bool;
    auto Contains(T key) -> bool;

    void Print() {
        SkipListNode<T> *p = LSentinel_;
        // print every layer
        for (int layer = this->max_layer_ - 1; layer >= 0; --layer) {
            std::cout << "layer " << layer << ": ";
            SkipListNode<T> *cur = p->next_[layer];
            while (cur != nullptr) {
                std::cout << cur->key_ << " ";
                cur = cur->next_[layer];
            }
            std::cout << std::endl;
        }
    }

private:

    auto FindNode(T key, SkipListNode<T> **preds, SkipListNode<T> **succs) -> int;

    SkipListNode<T> *LSentinel_;
};

// implementation
template <typename T>
NaiveSkipList<T>::NaiveSkipList(int max_layer, float p) : SkipList<T>(max_layer, p) {
    LSentinel_ = new SkipListNode<T>;
    LSentinel_->key_ = std::numeric_limits<T>::min();
    LSentinel_->top_layer_ = this->max_layer_ - 1;
    LSentinel_->next_ = new SkipListNode<T> *[this->max_layer_];
    for (int i = 0; i < this->max_layer_; ++i) {
        LSentinel_->next_[i] = nullptr;
    }
}

template <typename T>
NaiveSkipList<T>::~NaiveSkipList() {
    SkipListNode<T> *p = LSentinel_;
    SkipListNode<T> *q = nullptr;
    while (p != nullptr) {
        q = p->next_[0];
        delete[] p->next_;
        delete p;
        p = q;
    }
}

template <typename T>
auto NaiveSkipList<T>::FindNode(T key, SkipListNode<T> **preds, SkipListNode<T> **succs) -> int {
    SkipListNode<T> *p = LSentinel_;
    int lastFound = -1;
    for (int layer = this->max_layer_ - 1; layer >= 0; --layer) {
        SkipListNode<T> *cur = p->next_[layer];
        while (cur != nullptr && cur->key_ < key) {
            p = cur;
            cur = cur->next_[layer];
        }
        if (lastFound == -1 && cur != nullptr && cur->key_ == key) {
            lastFound = layer;
        }
        preds[layer] = p;
        succs[layer] = cur;
    }
    return lastFound;
}

template <typename T>
auto NaiveSkipList<T>::Add(T key) -> bool {
    SkipListNode<T> *preds[this->max_layer_];
    SkipListNode<T> *succs[this->max_layer_];
    int layer = FindNode(key, preds, succs);
    if (layer != -1) {
        return false;
    }
    auto *new_node = new SkipListNode<T>;
    new_node->key_ = key;
    new_node->top_layer_ = this->RandomLayer();
    new_node->next_ = new SkipListNode<T> *[new_node->top_layer_ + 1];
    for (int i = 0; i <= new_node->top_layer_; ++i) {
        new_node->next_[i] = succs[i];
        preds[i]->next_[i] = new_node;
    }
    return true;
}

template <typename T>
auto NaiveSkipList<T>::Remove(T key) -> bool {
    SkipListNode<T> *preds[this->max_layer_];
    SkipListNode<T> *succs[this->max_layer_];
    int layer = FindNode(key, preds, succs);
    if (layer == -1) {
        return false;
    }
    SkipListNode<T> *node_to_remove = succs[layer];
    for (int i = layer; i >= 0; --i) {
        preds[i]->next_[i] = node_to_remove->next_[i];
    }
    delete[] node_to_remove->next_;
    delete node_to_remove;
    return true;
}

template <typename T>
auto NaiveSkipList<T>::Contains(T key) -> bool {
    SkipListNode<T> *preds[this->max_layer_];
    SkipListNode<T> *succs[this->max_layer_];
    int layer = FindNode(key, preds, succs);
    return layer != -1;
}

#endif // NaiveSkipList_HPP