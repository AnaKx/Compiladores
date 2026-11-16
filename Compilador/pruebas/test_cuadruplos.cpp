// Pruebas de las pilas (operadores, operandos, tipos) y la fila de cuádruplos,
// construidas con Stack y Queue de la Tarea 1.
// Simula a mano lo que harán los puntos neurálgicos con:  x = (a + b) * c;

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

#include "cuadruplos.hpp"

template <typename F>
bool lanzaFueraDeRango(F accion) {
    try {
        accion();
    } catch (const std::out_of_range&) {
        return true;
    }
    return false;
}

int main() {
    // Stack: LIFO
    Stack<int> p;
    assert(p.empty());
    p.push(1);
    p.push(2);
    p.push(3);
    assert(p.size() == 3 && p.top() == 3);
    assert(sacar(p) == 3 && sacar(p) == 2 && p.top() == 1);
    assert(sacar(p) == 1 && p.empty());
    assert(lanzaFueraDeRango([&] { p.pop(); }));
    assert(lanzaFueraDeRango([&] { p.top(); }));

    // Queue: FIFO, con acceso por posición
    Queue<std::string> f;
    f.enqueue("a");
    f.enqueue("b");
    f.enqueue("c");
    assert(f.front() == "a" && f.at(2) == "c");
    f.dequeue();
    assert(f.front() == "b" && f.at(0) == "b" && f.size() == 2);
    assert(lanzaFueraDeRango([&] { f.at(2); }));
    f.dequeue();
    f.dequeue();
    assert(f.empty());
    assert(lanzaFueraDeRango([&] { f.dequeue(); }));
    assert(lanzaFueraDeRango([&] { f.front(); }));

    // at() sigue funcionando cuando el buffer circular da la vuelta y crece
    Queue<int> q;
    for (int i = 0; i < 6; i++) q.enqueue(i);
    for (int i = 0; i < 5; i++) q.dequeue();        // head_ queda en 5
    for (int i = 6; i < 20; i++) q.enqueue(i);      // da la vuelta y crece
    for (int i = 0; i < 15; i++) assert(q.at(i) == 5 + i);

    // x = (a + b) * c;   con x, a, b, c : int
    EstructurasCodigo e;
    e.pushOperando("x", Tipo::INT);
    e.poper.push(Op::ASIGNA);
    e.poper.push(Op::FONDO_FALSO);  // (
    e.pushOperando("a", Tipo::INT);
    e.poper.push(Op::SUMA);
    e.pushOperando("b", Tipo::INT);

    // ) : se resuelve lo que hay arriba del fondo falso
    while (e.poper.top() != Op::FONDO_FALSO) {
        Op op = sacar(e.poper);
        auto [der, tDer] = e.popOperando();
        auto [izq, tIzq] = e.popOperando();
        Tipo t = resultado(tIzq, tDer, op);
        assert(t == Tipo::INT);
        e.generar(simboloOp(op), izq, der, "t1");
        e.pushOperando("t1", t);
    }
    e.poper.pop();  // se quita el fondo falso

    e.poper.push(Op::MULT);
    e.pushOperando("c", Tipo::INT);
    {
        Op op = sacar(e.poper);
        auto [der, tDer] = e.popOperando();
        auto [izq, tIzq] = e.popOperando();
        e.generar(simboloOp(op), izq, der, "t2");
        e.pushOperando("t2", resultado(tIzq, tDer, op));
    }
    {
        Op op = sacar(e.poper);  // =
        auto [val, tVal] = e.popOperando();
        auto [dest, tDest] = e.popOperando();
        assert(resultado(tDest, tVal, op) == Tipo::INT);
        e.generar(simboloOp(op), val, "", dest);
    }
    assert(e.poper.empty() && e.pilaO.empty() && e.ptipos.empty());
    assert(e.cuadruplos.size() == 3);
    assert(e.cuadruplos.at(0).op == "+" && e.cuadruplos.at(0).res == "t1");
    assert(e.cuadruplos.at(1).op == "*" && e.cuadruplos.at(1).izq == "t1");
    assert(e.cuadruplos.at(2).op == "=" && e.cuadruplos.at(2).res == "x");

    // El fondo falso no es operador del cubo
    assert(resultado(Tipo::INT, Tipo::INT, Op::FONDO_FALSO) == Tipo::ERROR);

    // Salto pendiente que se rellena después
    std::size_t gotof = e.generar("GOTOF", "t3", "", "");
    e.generar("print", "", "", "x");
    e.rellenar(gotof, e.siguiente());
    assert(e.cuadruplos.at(gotof).res == "5");

    std::cout << "Cuádruplos:\n";
    e.imprimir(std::cout);

    // Se consumen en orden FIFO
    assert(e.cuadruplos.front().op == "+");
    e.cuadruplos.dequeue();
    assert(e.cuadruplos.front().op == "*");

    std::cout << "\nTodas las pruebas de pilas y fila pasaron.\n";
    return 0;
}
