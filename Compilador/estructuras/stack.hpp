// ============================================================================
// stack.hpp
// Estructura de apoyo (Tarea 1). No es una fase.
//
//   Autora: Ana Karina Aramoni Ruiz, 28/09/2026 (Tarea1_A07192068)
//
// Qué hace:
//   Pila genérica Stack<T> (LIFO: el último que entra es el primero que sale),
//   guardada en un std::vector: push, pop, top, empty, size, clear.
//
// Papel en el proceso:
//   Herramienta de la fase 4: cuadruplos.hpp la usa para la pila de operadores,
//   de operandos, de tipos y de saltos pendientes, y generador.hpp para las
//   llamadas a función abiertas.
//   No depende de ningún otro archivo del proyecto.
// ============================================================================

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
