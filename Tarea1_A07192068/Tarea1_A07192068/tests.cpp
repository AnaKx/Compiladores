//
//  tests.cpp
//  Tarea1_A07192068
//
//  Created by Ana Karina Aramoni Ruiz on 29/9/2026.
//

#include <climits> // INT_MAX, INT_MIN
#include <iostream>
#include <stdexcept>
#include <string>

#include "stack.hpp"
#include "queue.hpp"
#include "hash.hpp"

// mini test framework

int passed = 0;
int failed = 0;

// Imprime PASS o FAIL para una prueba y se cuenta.
void check(bool condition, const std::string& name) {
    if (condition) {
        ++passed;
        std::cout << "  [PASS] " << name << '\n';
    } else {
        ++failed;
        std::cout << "  [FAIL] " << name << '\n';
    }
}

template <typename F>
bool throws(F action) {
    try {
        action();
    } catch (const std::out_of_range&) {
        return true;
    }
    return false;
}

// STACK

void test_stack() {
    std::cout << "\n=== STACK ===\n";

    // ----- HAPPY PATH -----
    {
        Stack<int> s;
        s.push(5);
        check(s.top() == 5 && s.size() == 1, "S01 [HAPPY] push one value, top returns it");
    }
    {
        Stack<int> s;
        s.push(1); s.push(2); s.push(3);
        check(s.top() == 3, "S02 [HAPPY] top returns the LAST value pushed");
    }
    {
        Stack<int> s;
        s.push(1); s.push(2); s.push(3);
        bool ok = true;
        ok = ok && s.top() == 3; s.pop();
        ok = ok && s.top() == 2; s.pop();
        ok = ok && s.top() == 1; s.pop();
        check(ok && s.empty(), "S03 [HAPPY] pops come out in reverse order (LIFO)");
    }
    {
        Stack<int> s;
        s.push(7);
        int a = s.top();
        int b = s.top();
        check(a == 7 && b == 7 && s.size() == 1, "S04 [HAPPY] top does NOT remove the element");
    }
    {
        Stack<int> s;
        s.push(7);
        s.top() = 99; // top devuelve una referencia, por lo que  modifica el valor almacenado
        check(s.top() == 99, "S05 [HAPPY] top can modify the top value");
    }

    // ----- BOUNDARY -----
    {
        Stack<int> s;
        s.push(1);
        s.pop();
        check(s.empty() && s.size() == 0, "S06 [BOUNDARY] push 1, pop 1 -> back to empty");
    }
    {
        Stack<int> s;
        const int N = 100000;
        for (int i = 0; i < N; ++i) s.push(i);
        bool ok = s.size() == N;
        for (int i = N - 1; i >= 0; --i) {
            if (s.top() != i) ok = false;
            s.pop();
        }
        check(ok && s.empty(), "S07 [BOUNDARY] 100,000 pushes then pops, order correct");
    }
    {
        Stack<int> s;
        s.push(1); s.pop();
        s.push(2);
        check(s.top() == 2 && s.size() == 1, "S08 [BOUNDARY] stack is reusable after being emptied");
    }
    {
        Stack<int> s;
        s.push(INT_MIN); s.push(INT_MAX);
        bool ok = s.top() == INT_MAX;
        s.pop();
        check(ok && s.top() == INT_MIN, "S09 [BOUNDARY] stores the smallest and largest int");
    }
    {
        Stack<int> s;
        s.push(4); s.push(4); s.push(4);
        check(s.size() == 3 && s.top() == 4, "S10 [BOUNDARY] duplicate values are all kept");
    }

    // ----- EMPTY / NULL -----
    {
        Stack<int> s;
        check(s.empty() && s.size() == 0, "S11 [EMPTY] new stack is empty with size 0");
    }
    {
        Stack<int> s;
        check(throws([&] { s.pop(); }), "S12 [EMPTY] pop on empty stack throws out_of_range");
    }
    {
        Stack<int> s;
        check(throws([&] { s.top(); }), "S13 [EMPTY] top on empty stack throws out_of_range");
    }
    {
        Stack<int> s;
        s.push(1); s.pop();
        check(throws([&] { s.pop(); }), "S14 [EMPTY] pop on a stack that WAS emptied throws");
    }
    {
        Stack<int> s;
        s.push(1); s.push(2);
        s.clear();
        check(s.empty() && throws([&] { s.top(); }), "S15 [EMPTY] after clear, stack is empty and top throws");
    }
    {
        Stack<std::string> s;
        s.push("");
        check(!s.empty() && s.top() == "", "S16 [NULL] empty string is valid data and gets stored");
    }
    {
        Stack<int*> s;
        s.push(nullptr);
        check(s.size() == 1 && s.top() == nullptr, "S17 [NULL] nullptr can be stored as a value");
    }
}

// QUEUE

// Extrae todos los elementos de la fila y verifica que los valores obtenidos sean first, first+1, ..., last.
bool drains_in_order(Queue<int>& q, int first, int last) {
    for (int expected = first; expected <= last; ++expected) {
        if (q.empty() || q.front() != expected) return false;
        q.dequeue();
    }
    return q.empty();
}

void test_queue() {
    std::cout << "\n=== QUEUE ===\n";

    // ----- HAPPY PATH -----
    {
        Queue<int> q;
        q.enqueue(5);
        check(q.front() == 5 && q.size() == 1, "Q01 [HAPPY] enqueue one value, front returns it");
    }
    {
        Queue<int> q;
        q.enqueue(1); q.enqueue(2); q.enqueue(3);
        check(drains_in_order(q, 1, 3), "Q02 [HAPPY] values leave in arrival order (FIFO)");
    }
    {
        Queue<int> q;
        q.enqueue(7);
        int a = q.front();
        int b = q.front();
        check(a == 7 && b == 7 && q.size() == 1, "Q03 [HAPPY] front does NOT remove the element");
    }

    // ----- BOUNDARY -----
    {
        Queue<int> q;
        q.enqueue(1);
        q.dequeue();
        check(q.empty() && q.size() == 0, "Q04 [BOUNDARY] enqueue 1, dequeue 1 -> back to empty");
    }
    {
        Queue<int> q;
        for (int i = 0; i < 8; ++i) q.enqueue(i); // exactamente la capacidad inicial
        check(q.size() == 8 && drains_in_order(q, 0, 7), "Q05 [BOUNDARY] exactly 8 elements (full, no grow)");
    }
    {
        Queue<int> q;
        for (int i = 0; i < 9; ++i) q.enqueue(i); // se fuerza grow()
        check(q.size() == 9 && drains_in_order(q, 0, 8), "Q06 [BOUNDARY] 9th element triggers grow, order kept");
    }
    {
        Queue<int> q;
        for (int i = 0; i < 6; ++i) q.enqueue(i);
        for (int i = 0; i < 4; ++i) q.dequeue();// head_ se mueve a 4
        for (int i = 6; i < 12; ++i) q.enqueue(i); // se llena 6, 7 y WRAPS a 0, 1, 2, 3
        check(drains_in_order(q, 4, 11), "Q07 [BOUNDARY] wrap-around keeps FIFO order");
    }
    {
        Queue<int> q;
        for (int i = 0; i < 8; ++i) q.enqueue(i); // lleno
        for (int i = 0; i < 3; ++i) q.dequeue();
        for (int i = 8; i < 11; ++i) q.enqueue(i);
        q.enqueue(11);
        check(drains_in_order(q, 3, 11), "Q08 [BOUNDARY] grow while wrapped keeps FIFO order");
    }
    {
        Queue<int> q;
        const int N = 100000;
        for (int i = 0; i < N; ++i) q.enqueue(i);
        check(q.size() == N && drains_in_order(q, 0, N - 1), "Q09 [BOUNDARY] 100,000 elements, order correct");
    }
    {
        Queue<int> q;
        q.enqueue(1); q.dequeue();
        q.enqueue(2);
        check(q.front() == 2 && q.size() == 1, "Q10 [BOUNDARY] queue is reusable after being emptied");
    }

    // ----- EMPTY / NULL -----
    {
        Queue<int> q;
        check(q.empty() && q.size() == 0, "Q11 [EMPTY] new queue is empty with size 0");
    }
    {
        Queue<int> q;
        check(throws([&] { q.dequeue(); }), "Q12 [EMPTY] dequeue on empty queue throws out_of_range");
    }
    {
        Queue<int> q;
        check(throws([&] { q.front(); }), "Q13 [EMPTY] front on empty queue throws out_of_range");
    }
    {
        Queue<int> q;
        q.enqueue(1); q.dequeue();
        check(throws([&] { q.front(); }), "Q14 [EMPTY] front on a queue that WAS emptied throws");
    }
    {
        Queue<std::string> q;
        q.enqueue("");
        check(!q.empty() && q.front() == "", "Q15 [NULL] empty string is valid data and gets stored");
    }
    {
        Queue<int*> q;
        q.enqueue(nullptr);
        check(q.size() == 1 && q.front() == nullptr, "Q16 [NULL] nullptr can be stored as a value");
    }
}

// HASH TABLE

void test_hash() {
    std::cout << "\n=== HASH TABLE ===\n";

    // ----- HAPPY PATH -----
    {
        Hash<std::string, std::string> t;
        t.put("x", "int");
        std::string* v = t.find("x");
        check(v != nullptr && *v == "int", "H01 [HAPPY] put then find returns the value");
    }
    {
        Hash<std::string, std::string> t;
        t.put("x", "int"); t.put("y", "float"); t.put("nombre", "string");
        check(*t.find("x") == "int" && *t.find("y") == "float" && *t.find("nombre") == "string",
              "H02 [HAPPY] several keys, each finds its own value");
    }
    {
        Hash<std::string, std::string> t;
        t.put("x", "int");
        t.put("x", "float");
        check(*t.find("x") == "float" && t.size() == 1, "H03 [HAPPY] put on existing key updates, no duplicate");
    }
    {
        Hash<std::string, int> t;
        t.put("a", 1);
        check(t.contains("a"), "H04 [HAPPY] contains is true for a stored key");
    }
    {
        Hash<std::string, int> t;
        t.put("a", 1); t.put("b", 2);
        bool removed = t.erase("a");
        check(removed && !t.contains("a") && t.contains("b") && t.size() == 1,
              "H05 [HAPPY] erase removes only that key");
    }

    // ----- BOUNDARY -----
    {
        Hash<int, int> t;
        for (int i = 0; i < 9; ++i) t.put(i, i * 10);   // 8 drawers -> 9th triggers grow()
        bool ok = t.size() == 9;
        for (int i = 0; i < 9; ++i) ok = ok && t.find(i) != nullptr && *t.find(i) == i * 10;
        check(ok, "H06 [BOUNDARY] all keys still findable after the first grow");
    }
    {
        Hash<int, int> t;
        const int N = 10000;
        for (int i = 0; i < N; ++i) t.put(i, i);
        bool ok = t.size() == N;
        for (int i = 0; i < N; ++i) ok = ok && t.contains(i);
        check(ok, "H07 [BOUNDARY] 10,000 keys, many grows, all findable");
    }
    {
        // With 8 drawers, keys 0, 8, 16, 24 very likely share a drawer (collision).
        Hash<int, int> t;
        t.put(0, 100); t.put(8, 108); t.put(16, 116); t.put(24, 124);
        check(*t.find(0) == 100 && *t.find(8) == 108 && *t.find(16) == 116 && *t.find(24) == 124,
              "H08 [BOUNDARY] colliding keys are all stored and found");
    }
    {
        // Erase from the MIDDLE of a shared drawer: tests the "copy last into the gap" trick.
        Hash<int, int> t;
        t.put(0, 100); t.put(8, 108); t.put(16, 116);
        t.erase(8);
        check(!t.contains(8) && *t.find(0) == 100 && *t.find(16) == 116 && t.size() == 2,
              "H09 [BOUNDARY] erase from middle of a collision list keeps the others");
    }
    {
        Hash<std::string, int> t;
        t.put("a", 1);
        t.erase("a");
        t.put("a", 2);
        check(*t.find("a") == 2 && t.size() == 1, "H10 [BOUNDARY] erase then put the same key again");
    }
    {
        Hash<std::string, int> t;
        t.put("a", 1); t.put("b", 2); t.put("c", 3);
        t.erase("a"); t.erase("b"); t.erase("c");
        check(t.empty() && t.size() == 0, "H11 [BOUNDARY] erase every key -> table is empty");
    }
    {
        Hash<std::string, int> t;
        t.put("x", 1); t.put("X", 2);
        check(*t.find("x") == 1 && *t.find("X") == 2 && t.size() == 2,
              "H12 [BOUNDARY] keys are case-sensitive (x and X are different)");
    }
    {
        Hash<int, std::string> t;
        t.put(0, "cero"); t.put(-1, "menos uno"); t.put(INT_MAX, "max"); t.put(INT_MIN, "min");
        check(*t.find(0) == "cero" && *t.find(-1) == "menos uno" &&
              *t.find(INT_MAX) == "max" && *t.find(INT_MIN) == "min",
              "H13 [BOUNDARY] zero, negative, INT_MAX and INT_MIN work as keys");
    }

    // ----- EMPTY / NULL -----
    {
        Hash<std::string, int> t;
        check(t.empty() && t.size() == 0, "H14 [EMPTY] new table is empty with size 0");
    }
    {
        Hash<std::string, int> t;
        check(t.find("x") == nullptr, "H15 [EMPTY] find on empty table returns nullptr");
    }
    {
        Hash<std::string, int> t;
        check(!t.contains("x"), "H16 [EMPTY] contains on empty table is false");
    }
    {
        Hash<std::string, int> t;
        check(!t.erase("x") && t.size() == 0, "H17 [EMPTY] erase on empty table returns false, no crash");
    }
    {
        Hash<std::string, int> t;
        t.put("a", 1);
        check(t.find("zzz") == nullptr && !t.erase("zzz") && t.size() == 1,
              "H18 [EMPTY] find/erase a missing key: nullptr / false, size unchanged");
    }
    {
        Hash<std::string, int> t;
        t.put("a", 1);
        bool first = t.erase("a");
        bool second = t.erase("a");
        check(first && !second, "H19 [EMPTY] erasing the same key twice: true, then false");
    }
    {
        Hash<std::string, int> t;
        t.put("", 42);
        check(t.contains("") && *t.find("") == 42, "H20 [NULL] empty string works as a key");
    }
    {
        Hash<std::string, std::string> t;
        t.put("x", "");
        check(t.contains("x") && *t.find("x") == "", "H21 [NULL] empty string works as a value");
    }
    {
        // Important difference: find returns a pointer TO the value.
        // If the value itself is nullptr, find still returns a valid (non-null) pointer.
        Hash<std::string, int*> t;
        t.put("p", nullptr);
        int** v = t.find("p");
        check(t.contains("p") && v != nullptr && *v == nullptr,
              "H22 [NULL] nullptr stored as a value: key exists, value is nullptr");
    }
}

int main() {
    test_stack();
    test_queue();
    test_hash();

    std::cout << "\nResultado: " << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1; // non-zero exit code means "something failed"
}

