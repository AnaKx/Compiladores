// ============================================================================
// hash.hpp
// Estructura de apoyo (Tarea 1). No es una fase.
//
//   Autora: Ana Karina Aramoni Ruiz, 28/09/2026 (Tarea1_A07192068)
//
// Qué hace:
//   Tabla hash genérica Hash<K, V> (clave -> valor) con encadenamiento: put,
//   find, contains, erase, forEach. Busca en O(1) en promedio.
//
// Papel en el proceso:
//   Herramienta de las fases 3 y 4: tablas.hpp la usa para las tablas de
//   variables y de funciones, y memoria.hpp para la tabla de constantes.
//   No depende de ningún otro archivo del proyecto.
// ============================================================================

#pragma once
#include <cstddef>// std::size_t
#include <functional> // std::hash
#include <utility> // std::pair
#include <vector>// std::vector

template <typename K, typename V>
class Hash {
public:
    using Bucket = std::vector<std::pair<K, V>>;

    Hash() : buckets_(8) {}

    void put(const K& key, const V& value) {
        Bucket& bucket = buckets_[index_for(key)];
        for (std::size_t i = 0; i < bucket.size(); ++i) {
            if (bucket[i].first == key) {
                bucket[i].second = value; // update si key existe
                return;
            }
        }
        bucket.push_back({key, value});
        ++size_;
        if (size_ > buckets_.size()) grow();
    }

    V* find(const K& key) {
        Bucket& bucket = buckets_[index_for(key)];
        for (std::size_t i = 0; i < bucket.size(); ++i) {
            if (bucket[i].first == key) return &bucket[i].second;
        }
        return nullptr;
    }

    const V* find(const K& key) const {
        const Bucket& bucket = buckets_[index_for(key)];
        for (std::size_t i = 0; i < bucket.size(); ++i) {
            if (bucket[i].first == key) return &bucket[i].second;
        }
        return nullptr;
    }

    bool contains(const K& key) const { return find(key) != nullptr; }

    // Llama f(clave, valor) por cada par, sin orden particular
    template <typename F>
    void forEach(F f) const {
        for (const Bucket& bucket : buckets_)
            for (const auto& par : bucket) f(par.first, par.second);
    }

    bool erase(const K& key) {
        Bucket& bucket = buckets_[index_for(key)];
        for (std::size_t i = 0; i < bucket.size(); ++i) {
            if (bucket[i].first == key) {
                bucket[i] = bucket.back(); // mueve el último par al disponible
                bucket.pop_back();
                --size_;
                return true;
            }
        }
        return false;
    }

    // helpers
    
    std::size_t size() const { return size_; }
    
    bool empty() const { return size_ == 0; }

private:
    std::size_t index_for(const K& key) const {
        return std::hash<K>{}(key) % buckets_.size();
    }

    void grow() {
        std::vector<Bucket> old = buckets_;
        buckets_ = std::vector<Bucket>(old.size() * 2);
        for (std::size_t b = 0; b < old.size(); ++b)
            for (std::size_t i = 0; i < old[b].size(); ++i)
                buckets_[index_for(old[b][i].first)].push_back(old[b][i]);
    }

    std::vector<Bucket> buckets_;
    std::size_t size_ = 0;
};
