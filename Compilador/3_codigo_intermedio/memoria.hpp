// ============================================================================
// memoria.hpp
// Fase 4: código intermedio (también la usa la fase 3)
//
//   texto ─► scanner ─► tokens ─► parser ─► árbol ─► semántica ─► CUÁDRUPLOS
//
// Qué hace:
//   Reparte direcciones virtuales (números) a variables globales, locales,
//   temporales y constantes, en un rango distinto por segmento y tipo, y guarda
//   la tabla de constantes (cada valor repetido usa la misma dirección).
//
// Papel en el proceso:
//   Gracias a ella los cuádruplos llevan números en vez de nombres; con el número
//   se sabe el segmento y el tipo, que es lo que necesitará la máquina virtual.
//   tablas.hpp le pide direcciones al declarar variables y generador.hpp le pide
//   temporales y constantes.
//   Usa: cubo_semantico.hpp, hash.hpp.
// ============================================================================

#pragma once

#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "cubo_semantico.hpp"
#include "hash.hpp"

// Error de memoria si se acabo el espacio de un segmento
class ErrorMemoria : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

enum class Segmento { GLOBAL, LOCAL, TEMPORAL, CONSTANTE };

namespace memoria {
constexpr int TAM = 1000; // direcciones por segmento y tipo

constexpr int GLOBAL_INT = 1000;
constexpr int GLOBAL_FLOAT = 2000;
constexpr int LOCAL_INT = 3000;
constexpr int LOCAL_FLOAT = 4000;
constexpr int TEMP_INT = 5000;
constexpr int TEMP_FLOAT = 6000;
constexpr int TEMP_BOOL = 7000;
constexpr int CTE_INT = 8000;
constexpr int CTE_FLOAT = 9000;
constexpr int CTE_STRING = 10000;
constexpr int FIN = 11000;

constexpr int SIN_DIRECCION = -1; // operando con error de tipo (no se imprime hay errores)

// Segmento y tipo de una direccion
inline Segmento segmentoDe(int dir) {
    if (dir < LOCAL_INT) return Segmento::GLOBAL;
    if (dir < TEMP_INT) return Segmento::LOCAL;
    if (dir < CTE_INT) return Segmento::TEMPORAL;
    return Segmento::CONSTANTE;
}

// Tipo de la direccion; string regresan error porque no son tipo en el lenguaje
inline Tipo tipoDe(int dir) {
    switch ((dir / TAM) * TAM) {
        case GLOBAL_INT: case LOCAL_INT: case TEMP_INT: case CTE_INT: return Tipo::INT;
        case GLOBAL_FLOAT: case LOCAL_FLOAT: case TEMP_FLOAT: case CTE_FLOAT: return Tipo::FLOAT;
        case TEMP_BOOL: return Tipo::BOOL;
        default: return Tipo::ERROR;
    }
}
}

// Un rango [base, base + TAM) de un segmento y tipo; reparte direcciones en orden
class Contador {
public:
    Contador(int base, const char* nombre) : base(base), nombre(nombre) {}

    int asignar() {
        if (usadas >= memoria::TAM) {
            throw ErrorMemoria(std::string("se acabó la memoria de ") + nombre + " (máximo " +
                               std::to_string(memoria::TAM) + ")");
        }
        return base + usadas++;
    }

    void reiniciar() { usadas = 0; }
    int cuantas() const { return usadas; }

private:
    int base;
    const char* nombre;
    int usadas = 0;
};

// Cuantas direcciones de cada tipo usa un ambito para que ERA reserve su tamano
struct Recursos {
    int ints = 0;
    int floats = 0;
    int bools = 0;

    void contar(Tipo t) {
        if (t == Tipo::INT) ints++;
        if (t == Tipo::FLOAT) floats++;
        if (t == Tipo::BOOL) bools++;
    }
    int total() const { return ints + floats + bools; }
};

inline std::ostream& operator<<(std::ostream& os, const Recursos& r) {
    return os << r.ints << " int, " << r.floats << " float, " << r.bools << " bool";
}

class MemoriaVirtual {
public:
    int global(Tipo t) { return elegir(t, globalInt, globalFloat, nullptr).asignar(); }
    int local(Tipo t) { return elegir(t, localInt, localFloat, nullptr).asignar(); }
    int temporal(Tipo t) { return elegir(t, tempInt, tempFloat, &tempBool).asignar(); }

    // Al empezar una funcion/main locales y temporales vuelven al inicio de su segmento
    void nuevoAmbito() {
        localInt.reiniciar();
        localFloat.reiniciar();
        tempInt.reiniciar();
        tempFloat.reiniciar();
        tempBool.reiniciar();
    }

    // Constantes: el mismo valor siempre regresa la misma direccion
    int constante(const std::string& valor, Tipo t) {
        return buscarOAgregar(t == Tipo::FLOAT ? ctesFloat : ctesInt,
                              t == Tipo::FLOAT ? cteFloat : cteInt, valor);
    }
    int letrero(const std::string& texto) { return buscarOAgregar(ctesString, cteString, texto); }

    // Tabla de constantes en orden de direccion: (dirección, valor)
    std::vector<std::pair<int, std::string>> constantes() const {
        std::vector<std::pair<int, std::string>> res;
        for (const auto* tabla : {&ctesInt, &ctesFloat, &ctesString}) {
            tabla->forEach([&](const std::string& valor, int dir) { res.push_back({dir, valor}); });
        }
        std::sort(res.begin(), res.end()); // el Hash no guarda orden
        return res;
    }

    void imprimirConstantes(std::ostream& os, const std::string& sangria) const {
        auto ctes = constantes();
        if (ctes.empty()) os << sangria << "(vacía)\n";
        for (const auto& [dir, valor] : ctes) os << sangria << dir << " = " << valor << "\n";
    }

private:
    static Contador& elegir(Tipo t, Contador& i, Contador& f, Contador* b) {
        if (t == Tipo::INT) return i;
        if (t == Tipo::FLOAT) return f;
        if (t == Tipo::BOOL && b) return *b;
        throw ErrorMemoria("no hay segmento para el tipo " + nombreTipo(t));
    }

    static int buscarOAgregar(Hash<std::string, int>& tabla, Contador& c,
                              const std::string& valor) {
        if (const int* dir = tabla.find(valor)) return *dir;
        int dir = c.asignar();
        tabla.put(valor, dir);
        return dir;
    }

    Contador globalInt{memoria::GLOBAL_INT, "globales int"};
    Contador globalFloat{memoria::GLOBAL_FLOAT, "globales float"};
    Contador localInt{memoria::LOCAL_INT, "locales int"};
    Contador localFloat{memoria::LOCAL_FLOAT, "locales float"};
    Contador tempInt{memoria::TEMP_INT, "temporales int"};
    Contador tempFloat{memoria::TEMP_FLOAT, "temporales float"};
    Contador tempBool{memoria::TEMP_BOOL, "temporales bool"};
    Contador cteInt{memoria::CTE_INT, "constantes int"};
    Contador cteFloat{memoria::CTE_FLOAT, "constantes float"};
    Contador cteString{memoria::CTE_STRING, "letreros"};

    Hash<std::string, int> ctesInt, ctesFloat, ctesString;
};
