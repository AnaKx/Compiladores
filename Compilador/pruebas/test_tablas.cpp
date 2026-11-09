// Pruebas del Directorio de Funciones y las Tablas de Variables.
// Simula lo que hará el parser con pruebas/validos/completo.txt.

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
    assert(dir.buscarFuncion("noExiste") == nullptr);

    // Una local puede tener el mismo nombre que una global (la oculta)
    dir.agregarFuncion("otra", 12);
    dir.agregarLocal("i", Tipo::FLOAT, 13);
    assert(dir.buscarVariable("i")->tipo == Tipo::FLOAT);
    dir.terminarFuncion();
    assert(dir.buscarVariable("i")->tipo == Tipo::INT);

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
