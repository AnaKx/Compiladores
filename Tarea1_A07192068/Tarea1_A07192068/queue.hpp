//
//  queue.hpp
//  Tarea1_A07192068
//
//  Created by Ana Karina Aramoni Ruiz on 28/09/2026.
//
#pragma once
#include <stdexcept> // std::out_of_range
#include <cstddef> // std::size_t
#include <stdexcept> // std::out_of_range
#include <vector> // std::vector

template <typename T>
class Queue {
public:
    Queue() : buf_(8) {}
    
    void enqueue(const T& value) {
        if (size_ == buf_.size()) grow();
        buf_[(head_ + size_) % buf_.size()] = value; // wraps around to the front
        ++size_;
    }
    
    void dequeue() {
        if (empty()) throw std::out_of_range("No se puede hacer dequeue en una Queue vacía.");
        head_ = (head_ + 1) % buf_.size(); // pasa al siguiente elemento
        --size_;
    }
    
    T& front() {
        if (empty()) throw std::out_of_range("No se puede hacer front en una Queue vacía.");
        return buf_[head_];
    }
    
    // helpers
    
    bool empty() const { return size_ == 0; }

    std::size_t size() const { return size_; }
    
private:
    void grow() {
        std::vector<T> bigger(buf_.size() * 2);
        for (std::size_t i = 0; i < size_; ++i)
            bigger[i] = buf_[(head_ + i) % buf_.size()]; // copia de elementos en orden
        buf_ = bigger;
        head_ = 0;
    }
    
    std::vector<T> buf_;
    std::size_t head_ = 0;
    std::size_t size_ = 0;
};
