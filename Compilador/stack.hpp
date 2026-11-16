//
//  stack.hpp
//  Tarea1_A07192068
//
//  Created by Ana Karina Aramoni Ruiz on 28/09/2026.
//

#pragma once
#include <cstddef> // std::size_t
#include <stdexcept> // std::out_of_range
#include <vector> // std::vector

template <typename T>
class Stack {
public:
    void push(const T& value) {
        data_.push_back(value);
    }
    
    void pop() {
        if (empty()) throw std::out_of_range("No se puede hacer pop en un Stack vacío.");
        data_.pop_back();
    }
    
    T& top() {
        if (empty()) throw std::out_of_range("No se puede hacer top en un Stack vacío.");
        return data_.back(); // ultimo elemento en el vector
    }
    
    // helpers
    
    bool empty() const { return data_.empty(); }
    
    std::size_t size() const { return data_.size(); }
    
    void clear() { data_.clear(); }
    
private:
    std::vector<T> data_;
};
