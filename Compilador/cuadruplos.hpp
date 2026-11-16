// Pilas y Fila para la generación de código intermedio (cuádruplos) de BabyDuck.
//
// Usa las plantillas Stack (stack.hpp) y Queue (queue.hpp):
//
//   POper  : Stack<Op>           operadores pendientes (+ - * / > < =) y fondo falso '('
//   PilaO  : Stack<std::string>  operandos (nombre de variable, constante o temporal)
//   PTipos : Stack<Tipo>         tipo de cada operando de PilaO (van en paralelo)
//   PSaltos: Stack<size_t>       cuádruplos con salto pendiente y puntos de regreso
//   Quads  : Queue<Cuadruplo>    cuádruplos generados, en orden de ejecución
//
// La fila se llena por el final (FIFO); además Queue::at permite acceso por
// posición para "rellenar" el destino de un salto (GOTOF/GOTO) cuando ya se conoce.

#pragma once

#include <cstddef>
#include <iomanip>
#include <ostream>
#include <string>
#include <utility>

#include "cubo_semantico.hpp"
#include "queue.hpp"
#include "stack.hpp"

// Saca el tope de una pila y lo regresa (Stack::pop no regresa el elemento)
template <typename T>
T sacar(Stack<T>& pila) {
    T valor = pila.top();
    pila.pop();
    return valor;
}

// Cuadruplo: (operador, operando izquierdo, operando derecho, resultado).
// Los campos que no se usan quedan vacíos (se imprimen como "_").
struct Cuadruplo {
    std::string op;
    std::string izq;
    std::string der;
    std::string res;
};

// Conjunto de estructuras que usan los puntos neuralgicos de expresiones y estatutos
struct EstructurasCodigo {
    Stack<Op> poper;
    Stack<std::string> pilaO;
    Stack<Tipo> ptipos;
    Stack<std::size_t> psaltos;
    Queue<Cuadruplo> cuadruplos;

    // Un operando siempre entra y sale junto con su tipo
    void pushOperando(const std::string& nombre, Tipo tipo) {
        pilaO.push(nombre);
        ptipos.push(tipo);
    }

    std::pair<std::string, Tipo> popOperando() {
        std::string nombre = sacar(pilaO);
        return {nombre, sacar(ptipos)};
    }

    // Genera un cuadruplo y regresa su indice (el "contador" de cuadruplos)
    std::size_t generar(const std::string& op, const std::string& izq,
                        const std::string& der, const std::string& res) {
        cuadruplos.enqueue(Cuadruplo{op, izq, der, res});
        return cuadruplos.size() - 1;
    }

    // Indice que tendrq el siguiente cuadruplo
    std::size_t siguiente() const { return cuadruplos.size(); }

    // Pone el destino de un salto ya generado
    void rellenar(std::size_t indice, std::size_t destino) {
        cuadruplos.at(indice).res = std::to_string(destino);
    }

    void imprimir(std::ostream& os) const {
        for (std::size_t i = 0; i < cuadruplos.size(); i++) {
            const Cuadruplo& c = cuadruplos.at(i);
            os << std::setw(4) << i << ": (" << campo(c.op) << ", " << campo(c.izq) << ", "
               << campo(c.der) << ", " << campo(c.res) << ")\n";
        }
    }

private:
    static const std::string& campo(const std::string& s) {
        static const std::string vacio = "_";
        return s.empty() ? vacio : s;
    }
};
