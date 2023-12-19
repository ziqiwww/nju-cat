/**
 * This is a c++ implementation of skip list introduced in 
 * [1] M. Herlihy, Y. Lev, V. Luchangco, and N. Shavit, “A Simple Optimistic Skiplist Algorithm,” in Structural Information and Communication Complexity, vol. 4474, G. Prencipe and S. Zaks, Eds., in Lecture Notes in Computer Science, vol. 4474. , Berlin, Heidelberg: Springer Berlin Heidelberg, 2007, pp. 124–138. doi: 10.1007/978-3-540-72951-8_11.
 **/

#ifndef CONSKIPLIST_HPP
#define CONSKIPLIST_HPP

#include "SkipList.hpp"
#include <mutex>
#include <atomic>
#include <iostream>

template<typename T>
class ConSkipListNode {
public:
    T key_;
    int top_layer_;
    std::vector<std::shared_ptr<ConSkipListNode<T>>> next_;
    std::atomic<bool> marked_;
    std::atomic<bool> fully_linked_;
    std::mutex lock_;

    ConSkipListNode(T key, int top_layer) {
        key_ = key;
        top_layer_ = top_layer;
        next_.resize(top_layer + 1);
        marked_ = false;
        fully_linked_ = false;
    }
};

template<typename T>
class ConSkipList : public SkipList<T> {
public:
    ConSkipList(int max_layer, float p);

    ~ConSkipList();

    auto Add(T key) -> bool;

    auto Remove(T key) -> bool;

    auto Contains(T key) -> bool;

    void Print() {
        std::shared_ptr<ConSkipListNode<T>> p = LSentinel_;
        // print every layer
        for (int layer = this->max_layer_ - 1; layer >= 0; --layer) {
            std::cout << "layer " << layer << ": ";
            std::shared_ptr<ConSkipListNode<T>> cur = p->next_[layer];
            while (cur != RSentinel_) {
                std::cout << cur->key_ << " ";
                cur = cur->next_[layer];
            }
            std::cout << std::endl;
        }
    }

private:
    auto FindNode(T key, std::shared_ptr<ConSkipListNode<T>> *preds, std::shared_ptr<ConSkipListNode<T>> *succs) -> int;

    std::shared_ptr<ConSkipListNode<T>> LSentinel_;

    std::shared_ptr<ConSkipListNode<T>> RSentinel_;
};

// implementation
template<typename T>
ConSkipList<T>::ConSkipList(int max_layer, float p) : SkipList<T>(max_layer, p) {
    LSentinel_ = std::make_shared<ConSkipListNode<T>>(std::numeric_limits<T>::min(), this->max_layer_ - 1);
    RSentinel_ = std::make_shared<ConSkipListNode<T>>(std::numeric_limits<T>::max(), this->max_layer_ - 1);
    for (int i = 0; i < this->max_layer_; ++i) {
        LSentinel_->next_[i] = RSentinel_;
        RSentinel_->next_[i] = nullptr;
    }
}

template<typename T>
ConSkipList<T>::~ConSkipList() {
    std::shared_ptr<ConSkipListNode<T>> p = LSentinel_;
    std::shared_ptr<ConSkipListNode<T>> q;
    while (p != nullptr) {
        q = p->next_[0];
        p->next_.clear();
        p = q;
    }
}


template<typename T>
auto ConSkipList<T>::FindNode(T key, std::shared_ptr<ConSkipListNode<T>> *preds,
                              std::shared_ptr<ConSkipListNode<T>> *succs) -> int {
    int layer = -1;
    std::shared_ptr<ConSkipListNode<T>> pred = LSentinel_;
    for (int i = this->max_layer_ - 1; i >= 0; --i) {
        std::shared_ptr<ConSkipListNode<T>> curr = pred->next_[i];
        // if we add RSentinel_ to the end of the list, curr is never nullptr
        while (curr != nullptr && curr->key_ < key) {
            pred = curr;
            curr = curr->next_[i];
        }
        if (layer == -1 && curr != nullptr && curr->key_ == key) {
            layer = i;
        }
        preds[i] = pred;
        succs[i] = curr;
    }
    return layer;
}

template<typename T>
auto ConSkipList<T>::Add(T key) -> bool {
    std::shared_ptr<ConSkipListNode<T>> preds[this->max_layer_];
    std::shared_ptr<ConSkipListNode<T>> succs[this->max_layer_];
    while (true) {
        int layer_check = FindNode(key, preds, succs);
        if (layer_check != -1) {
            std::shared_ptr<ConSkipListNode<T>> nodeFound = succs[layer_check];
            if (!nodeFound->marked_) {
                // wait until node is fully linked, i.e. node
                while (!nodeFound->fully_linked_) {}
                return false;
            }
            continue;
        }
        int highestLocked = -1;
        int top_layer = this->RandomLayer();
        std::shared_ptr<ConSkipListNode<T>> pred, succ, prevPred = nullptr;
        bool valid = true;
        for (int layer = 0; valid && layer <= top_layer; ++layer) {
            pred = preds[layer];
            succ = succs[layer];
            // check if pred and succ are still valid
            if (pred != prevPred) {
                pred->lock_.lock();
                highestLocked = layer;
                prevPred = pred;
            }
            valid = !pred->marked_ && !succ->marked_ && pred->next_[layer] == succ;
        }
        if (!valid) {
            for (int layer = 0; layer <= highestLocked; ++layer) {
                preds[layer]->lock_.unlock();
            }
            continue;
        }
        std::shared_ptr<ConSkipListNode<T>> newNode = std::make_shared<ConSkipListNode<T>>(key, top_layer);
        for (int layer = 0; layer <= newNode->top_layer_; ++layer) {
            newNode->next_[layer] = succs[layer];
            preds[layer]->next_[layer] = newNode;
        }
        // linearization point
        newNode->fully_linked_ = true;
        for (int layer = 0; layer <= highestLocked; ++layer) {
            preds[layer]->lock_.unlock();
        }
        return true;
    }
}

template<typename T>
auto ConSkipList<T>::Remove(T key) -> bool {
    std::shared_ptr<ConSkipListNode<T>> victim = nullptr;
    bool isMarked = false;
    std::shared_ptr<ConSkipListNode<T>> preds[this->max_layer_];
    std::shared_ptr<ConSkipListNode<T>> succs[this->max_layer_];
    while (true) {
        int layer_check = FindNode(key, preds, succs);
        if (layer_check != -1) {
            victim = succs[layer_check];
        }
        /**
         * bool okToDelete(Node* candidate, int lFound) {
         *    return candidate−>fullyLinked && candidate−>topLayer==lFound && ! candidate−>marked;
         * }
         */
        if (isMarked ||
            (layer_check != -1 &&
             victim->fully_linked_ && victim->top_layer_ == layer_check && !victim->marked_)) {
            if (!isMarked) {
                victim->lock_.lock();
                if (victim->marked_) {
                    victim->lock_.unlock();
                    return false;
                }
                victim->marked_ = true;
                isMarked = true;
            }
            int highestLocked = -1;
            std::shared_ptr<ConSkipListNode<T>> pred, succ, prevPred = nullptr;
            bool valid = true;
            for (int layer = 0; valid && layer <= victim->top_layer_; ++layer) {
                pred = preds[layer];
                succ = succs[layer];
                if (pred != prevPred) {
                    pred->lock_.lock();
                    highestLocked = layer;
                    prevPred = pred;
                }
                valid = !pred->marked_ && pred->next_[layer] == succ;
            }
            if (!valid) {
                for (int layer = 0; layer <= highestLocked; ++layer) {
                    preds[layer]->lock_.unlock();
                }
                continue;
            }
            for (int layer = victim->top_layer_; layer >= 0; --layer) {
                preds[layer]->next_[layer] = victim->next_[layer];
            }
            victim->lock_.unlock();
            for (int layer = 0; layer <= highestLocked; ++layer) {
                preds[layer]->lock_.unlock();
            }
            return true;
        } else {
            return false;
        }
    }
}

template<typename T>
auto ConSkipList<T>::Contains(T key) -> bool {
    std::shared_ptr<ConSkipListNode<T>> preds[this->max_layer_];
    std::shared_ptr<ConSkipListNode<T>> succs[this->max_layer_];
    int layer = FindNode(key, preds, succs);
    return (layer != -1 && succs[layer]->fully_linked_ && !succs[layer]->marked_);
}


#endif // CONSKIPLIST_HPP