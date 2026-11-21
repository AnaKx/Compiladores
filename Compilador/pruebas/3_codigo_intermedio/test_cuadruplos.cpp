// ============================================================================
// test_cuadruplos.cpp
// Pruebas de la fase 4 (no es parte del compilador)
//
//   texto ─► scanner ─► tokens ─► parser ─► árbol ─► semántica ─► CUÁDRUPLOS
//
// Qué hace:
//   Prueba stack.hpp, queue.hpp, cuadruplos.hpp y memoria.hpp directamente, sin
//   ANTLR: genera a mano los cuádruplos de x = (a + b) * c, rellena un salto y
//   revisa los rangos de la memoria virtual.
//
// Papel en el proceso:
//   Verifica la fase 4 por separado del resto. Se compila con
//   `make test_cuadruplos` y corre con `make test`. Si termina, todas las
//   pruebas pasaron.
// ============================================================================

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

#include "cuadruplos.hpp"
#include "memoria.hpp"

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
    e.pushOperando(1000, Tipo::INT); // x
    e.poper.push(Op::ASIGNA);
    e.poper.push(Op::FONDO_FALSO);  // (
    e.pushOperando(1001, Tipo::INT); // a
    e.poper.push(Op::SUMA);
    e.pushOperando(1002, Tipo::INT); // b

    // ) : se resuelve lo que hay arriba del fondo falso
    while (e.poper.top() != Op::FONDO_FALSO) {
        Op op = sacar(e.poper);
        auto [der, tDer] = e.popOperando();
        auto [izq, tIzq] = e.popOperando();
        Tipo t = resultado(tIzq, tDer, op);
        assert(t == Tipo::INT);
        e.generar(simboloOp(op), std::to_string(izq), std::to_string(der), "5000");
        e.pushOperando(5000, t);
    }
    e.poper.pop();  // se quita el fondo falso

    e.poper.push(Op::MULT);
    e.pushOperando(1003, Tipo::INT); // c
    {
        Op op = sacar(e.poper);
        auto [der, tDer] = e.popOperando();
        auto [izq, tIzq] = e.popOperando();
        e.generar(simboloOp(op), std::to_string(izq), std::to_string(der), "5001");
        e.pushOperando(5001, resultado(tIzq, tDer, op));
    }
    {
        Op op = sacar(e.poper);  // =
        auto [val, tVal] = e.popOperando();
        auto [dest, tDest] = e.popOperando();
        assert(resultado(tDest, tVal, op) == Tipo::INT);
        e.generar(simboloOp(op), std::to_string(val), "", std::to_string(dest));
    }
    assert(e.poper.empty() && e.pilaO.empty() && e.ptipos.empty());
    assert(e.cuadruplos.size() == 3);
    assert(e.cuadruplos.at(0).op == "+" && e.cuadruplos.at(0).izq == "1001" &&
           e.cuadruplos.at(0).res == "5000");
    assert(e.cuadruplos.at(1).op == "*" && e.cuadruplos.at(1).izq == "5000");
    assert(e.cuadruplos.at(2).op == "=" && e.cuadruplos.at(2).res == "1000");

    // El fondo falso no es operador del cubo
    assert(resultado(Tipo::INT, Tipo::INT, Op::FONDO_FALSO) == Tipo::ERROR);

    // Salto pendiente que se rellena después
    std::size_t gotof = e.generar("GOTOF", "7000", "", "");
    e.generar("print", "", "", "1000");
    e.rellenar(gotof, e.siguiente());
    assert(e.cuadruplos.at(gotof).res == "5");

    std::cout << "Cuádruplos:\n";
    e.imprimir(std::cout);

    // Se consumen en orden FIFO
    assert(e.cuadruplos.front().op == "+");
    e.cuadruplos.dequeue();
    assert(e.cuadruplos.front().op == "*");

    // Memoria virtual: cada segmento y tipo tiene su rango
    MemoriaVirtual m;
    assert(m.global(Tipo::INT) == 1000 && m.global(Tipo::INT) == 1001);
    assert(m.global(Tipo::FLOAT) == 2000);
    assert(m.local(Tipo::INT) == 3000 && m.local(Tipo::FLOAT) == 4000);
    assert(m.temporal(Tipo::INT) == 5000 && m.temporal(Tipo::FLOAT) == 6000);
    assert(m.temporal(Tipo::BOOL) == 7000 && m.temporal(Tipo::BOOL) == 7001);
    // Las constantes repetidas comparten dirección
    assert(m.constante("5", Tipo::INT) == 8000 && m.constante("7", Tipo::INT) == 8001);
    assert(m.constante("5", Tipo::INT) == 8000);
    assert(m.constante("2.5", Tipo::FLOAT) == 9000 && m.letrero("\"hola\"") == 10000);
    assert(m.constantes().size() == 4 && m.constantes()[2].second == "2.5");
    // Nuevo ambito: locales y temporales empiezan de nuevo, globales no
    m.nuevoAmbito();
    assert(m.local(Tipo::INT) == 3000 && m.temporal(Tipo::BOOL) == 7000);
    assert(m.global(Tipo::INT) == 1002);
    // Segmento y tipo se deducen de la dirección
    assert(memoria::segmentoDe(1500) == Segmento::GLOBAL && memoria::segmentoDe(4999) == Segmento::LOCAL);
    assert(memoria::segmentoDe(7000) == Segmento::TEMPORAL && memoria::segmentoDe(10000) == Segmento::CONSTANTE);
    assert(memoria::tipoDe(2001) == Tipo::FLOAT && memoria::tipoDe(7003) == Tipo::BOOL);
    assert(memoria::tipoDe(8000) == Tipo::INT && memoria::tipoDe(10000) == Tipo::ERROR);
    // Un segmento lleno es un error
    bool lleno = false;
    try {
        for (int i = 0; i <= memoria::TAM; i++) m.temporal(Tipo::FLOAT);
    } catch (const ErrorMemoria& err) {
        lleno = true;
        std::cout << "  esperado -> " << err.what() << "\n";
    }
    assert(lleno);
    // bool no tiene segmento global
    bool sinSegmento = false;
    try { m.global(Tipo::BOOL); } catch (const ErrorMemoria&) { sinSegmento = true; }
    assert(sinSegmento);

    std::cout << "\nTodas las pruebas de pilas, fila y memoria pasaron.\n";
    return 0;
}
