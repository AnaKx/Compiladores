// ============================================================================
// test_tablas.cpp
// Pruebas de la fase 3 (no es parte del compilador)
//
//   texto ─► scanner ─► tokens ─► parser ─► árbol ─► SEMÁNTICA ─► cuádruplos
//
// Qué hace:
//   Prueba hash.hpp y tablas.hpp directamente, sin ANTLR: llama a mano a las
//   acciones del directorio como lo haría la semántica (simula
//   pruebas/validos/completo.txt) y revisa con assert direcciones, ámbitos y
//   errores esperados.
//
// Papel en el proceso:
//   Verifica la fase 3 por separado del resto. Se compila con `make test_tablas`
//   y corre con `make test`. Si termina, todas las pruebas pasaron.
// ============================================================================

#include <cassert>
#include <iostream>
#include <string>

#include "tablas.hpp"

// Regresa true si la acción lanza ErrorSemantico.
template <typename F>
bool lanzaError(F accion) {
    try {
        accion();
    } catch (const ErrorSemantico& e) {
        std::cout << "  esperado -> línea " << e.linea << ": " << e.what() << "\n";
        return true;
    }
    return false;
}

int main() {
    // Hash de la Tarea 1 (lo usan las tablas): put/find/contains, crecimiento,
    // find const y forEach
    Hash<std::string, int> h;
    for (int i = 0; i < 50; i++) h.put("v" + std::to_string(i), i);  // crece varias veces
    assert(h.size() == 50 && *h.find("v37") == 37 && h.find("x") == nullptr);
    h.put("v37", 99);  // actualizar no agrega
    assert(h.size() == 50 && *h.find("v37") == 99);
    const Hash<std::string, int>& hc = h;
    assert(hc.contains("v0") && *hc.find("v49") == 49);
    int suma = 0, cuantos = 0;
    hc.forEach([&](const std::string&, int v) { suma += v; cuantos++; });
    assert(cuantos == 50 && suma == 49 * 50 / 2 - 37 + 99);

    DirectorioFunciones dir;

    // program demo;  var i, n : int;  var prom : float;
    dir.iniciarPrograma("demo");
    dir.agregarGlobal("i", Tipo::INT, 2);
    dir.agregarGlobal("n", Tipo::INT, 2);
    dir.agregarGlobal("prom", Tipo::FLOAT, 3);

    // void imprimeSuma(a : int, b : int) [ var s : int; ... ];
    dir.agregarFuncion("imprimeSuma", 5);
    dir.agregarParametro("a", Tipo::INT, 5);
    dir.agregarParametro("b", Tipo::INT, 5);
    dir.agregarLocal("s", Tipo::INT, 6);

    // Dentro de la función se ven locales y globales
    assert(dir.buscarVariable("s")->tipo == Tipo::INT);
    assert(dir.buscarVariable("prom")->tipo == Tipo::FLOAT);
    dir.terminarFuncion();

    // En main ya no se ven las locales de imprimeSuma
    assert(dir.buscarVariable("s") == nullptr);
    assert(dir.buscarVariable("i") != nullptr);

    const Funcion* f = dir.buscarFuncion("imprimeSuma");
    assert(f && f->parametros.size() == 2 && f->variables.tamano() == 3);

    // Direcciones virtuales: globales en 1000/2000, locales en 3000/4000
    assert(dir.buscarVariable("i")->direccion == 1000);
    assert(dir.buscarVariable("n")->direccion == 1001);
    assert(dir.buscarVariable("prom")->direccion == 2000);
    assert(f->variables.buscar("a")->direccion == 3000);
    assert(f->variables.buscar("s")->direccion == 3002);
    assert(f->dirParametros.size() == 2 && f->dirParametros[1] == 3001);
    assert(f->locales.ints == 3 && f->locales.floats == 0);
    assert(dir.buscarFuncion("noExiste") == nullptr);

    // Una local puede tener el mismo nombre que una global (la oculta)
    // Cada función empieza sus locales desde el inicio del segmento; con tipo, su
    // valor de regreso es una global
    dir.agregarFuncion("otra", 12);
    dir.fijarRetorno(Tipo::INT, 12);
    dir.agregarLocal("i", Tipo::FLOAT, 13);
    assert(dir.buscarVariable("i")->tipo == Tipo::FLOAT);
    assert(dir.buscarVariable("i")->direccion == 4000);
    assert(dir.buscarFuncion("otra")->direccionRetorno == 1002);
    assert(dir.temporal(Tipo::BOOL, 14) == 7000 && dir.temporal(Tipo::INT, 14) == 5000);
    assert(dir.buscarFuncion("otra")->temporales.total() == 2);
    dir.terminarFuncion();
    assert(dir.buscarVariable("i")->tipo == Tipo::INT);
    // main: sus temporales también empiezan desde el inicio
    assert(dir.temporal(Tipo::INT, 15) == 5000 && dir.recursosMain().ints == 1);
    assert(dir.constante("10", Tipo::INT, 15) == 8000 && dir.constante("10", Tipo::INT, 16) == 8000);
    assert(dir.recursosGlobal().ints == 3 && dir.recursosGlobal().floats == 1);

    std::cout << "Errores detectados:\n";
    assert(lanzaError([&] { dir.agregarGlobal("n", Tipo::FLOAT, 20); }));        // global duplicada
    assert(lanzaError([&] { dir.agregarGlobal("demo", Tipo::INT, 21); }));       // = programa
    assert(lanzaError([&] { dir.agregarGlobal("otra", Tipo::INT, 22); }));       // = función
    assert(lanzaError([&] { dir.agregarFuncion("imprimeSuma", 23); }));          // función duplicada
    assert(lanzaError([&] { dir.agregarFuncion("prom", 24); }));                 // = global
    assert(lanzaError([&] { dir.agregarFuncion("demo", 25); }));                 // = programa
    assert(lanzaError([&] { dir.agregarLocal("x", Tipo::INT, 26); }));           // sin función abierta
    assert(lanzaError([&] { dir.variable("zzz", 27); }));                        // no declarada

    dir.agregarFuncion("f", 30);
    dir.agregarParametro("p", Tipo::INT, 30);
    assert(lanzaError([&] { dir.agregarParametro("p", Tipo::FLOAT, 30); }));     // param duplicado
    assert(lanzaError([&] { dir.agregarLocal("p", Tipo::INT, 31); }));           // local = param
    dir.terminarFuncion();

    std::cout << "\nDirectorio:\n";
    dir.imprimir(std::cout);
    std::cout << "\nTodas las pruebas de tablas pasaron.\n";
    return 0;
}
