// Cubo semantico de BabyDuck

#pragma once

#include <string>

enum class Tipo { INT, FLOAT, BOOL, ERROR };
enum class Op { SUMA, RESTA, MULT, DIV, MAYOR, MENOR, ASIGNA };

constexpr int NUM_TIPOS = 4;
constexpr int NUM_OPS = 7;

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
