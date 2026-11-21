// ============================================================================
// cubo_semantico.hpp
// Fase 3: análisis semántico (también lo usa la fase 4)
//
//   texto ─► scanner ─► tokens ─► parser ─► árbol ─► SEMÁNTICA ─► cuádruplos
//
// Qué hace:
//   Define los tipos del lenguaje (int, float, bool, error) y los operadores
//   (+ - * / > < =), y la tabla CUBO[izq][der][op] que dice qué tipo resulta de
//   cada combinación, o ERROR si no se permite (p. ej. bool + int, int = float).
//   También convierte tipos y operadores a texto para mensajes y cuádruplos.
//
// Papel en el proceso:
//   Es la "regla" de compatibilidad de tipos. generador.hpp la consulta cada vez
//   que genera una operación, una asignación, un PARAM o un RETURN.
//   Los enums Tipo y Op se usan en todo el compilador.
//   No depende de ningún otro archivo del proyecto.
// ============================================================================

#pragma once

#include <string>

enum class Tipo { INT, FLOAT, BOOL, ERROR };
// FONDO_FALSO no es operador del cubo: es la marca '(' que se mete a la pila de operadores
enum class Op { SUMA, RESTA, MULT, DIV, MAYOR, MENOR, ASIGNA, FONDO_FALSO };

constexpr int NUM_TIPOS = 4;
constexpr int NUM_OPS = 7; // solo los operadores del cubo (sin FONDO_FALSO)

namespace cubo_detalle {
constexpr Tipo I = Tipo::INT;
constexpr Tipo F = Tipo::FLOAT;
constexpr Tipo B = Tipo::BOOL;
constexpr Tipo E = Tipo::ERROR;

// CUBO[izq][der][op]
// op: SUMA RESTA MULT DIV  MAYOR MENOR  ASIGNA
// En ASIGNA, izq = tipo de la variable destino, der = tipo de la expresión;
// el resultado es el tipo que queda guardado (ERROR si no es compatible).
constexpr Tipo CUBO[NUM_TIPOS][NUM_TIPOS][NUM_OPS] = {
    // izq = INT
    {
        /* der INT */ {I, I, I, I, B, B, I},
        /* der FLOAT */ {F, F, F, F, B, B, E},
        /* der BOOL */ {E, E, E, E, E, E, E},
        /* der ERROR */ {E, E, E, E, E, E, E},
    },
    // izq = FLOAT
    {
        /* der INT */ {F, F, F, F, B, B, F},
        /* der FLOAT */ {F, F, F, F, B, B, F},
        /* der BOOL */ {E, E, E, E, E, E, E},
        /* der ERROR */ {E, E, E, E, E, E, E},
    },
    // izq = BOOL
    {
        /* der INT */ {E, E, E, E, E, E, E},
        /* der FLOAT */ {E, E, E, E, E, E, E},
        /* der BOOL */ {E, E, E, E, E, E, E},
        /* der ERROR */ {E, E, E, E, E, E, E},
    },
    // izq = ERROR (el error se propaga)
    {
        {E, E, E, E, E, E, E},
        {E, E, E, E, E, E, E},
        {E, E, E, E, E, E, E},
        {E, E, E, E, E, E, E},
    },
};
}  // namespace cubo_detalle

constexpr Tipo resultado(Tipo izq, Tipo der, Op op) {
    if (static_cast<int>(op) >= NUM_OPS) return Tipo::ERROR;
    return cubo_detalle::CUBO[static_cast<int>(izq)][static_cast<int>(der)][static_cast<int>(op)];
}

// Signo unario (+x, -x): conserva el tipo numerico
constexpr Tipo resultadoUnario(Tipo t) {
    return (t == Tipo::INT || t == Tipo::FLOAT) ? t : Tipo::ERROR;
}

inline std::string nombreTipo(Tipo t) {
    switch (t) {
        case Tipo::INT: return "int";
        case Tipo::FLOAT: return "float";
        case Tipo::BOOL: return "bool";
        default: return "error";
    }
}

// Simbolo del operador (se usa como campo 'op' de los cuadruplos)
inline std::string simboloOp(Op op) {
    switch (op) {
        case Op::SUMA: return "+";
        case Op::RESTA: return "-";
        case Op::MULT: return "*";
        case Op::DIV: return "/";
        case Op::MAYOR: return ">";
        case Op::MENOR: return "<";
        case Op::ASIGNA: return "=";
        default: return "(";
    }
}
