// ============================================================================
// tablas.hpp
// Fase 3: análisis semántico
//
//   texto ─► scanner ─► tokens ─► parser ─► árbol ─► SEMÁNTICA ─► cuádruplos
//
// Qué hace:
//   Es la "libreta" del compilador: guarda el nombre del programa, las variables
//   globales y, por cada función, sus parámetros, variables locales, tipo de
//   retorno y el cuádruplo donde empieza. Rechaza nombres repetidos y busca
//   variables respetando el ámbito (primero la función actual, luego globales).
//
// Papel en el proceso:
//   semantica.hpp lo llena con las declaraciones; generador.hpp lo consulta para
//   saber el tipo y la dirección de cada nombre. Al dar de alta una variable le
//   pide su dirección a memoria.hpp. Con --tablas se imprime completo.
//   Recibe: altas y búsquedas de nombres.  Entrega: Variable / Funcion o un
//   ErrorSemantico con la línea.
//   Usa: hash.hpp, cubo_semantico.hpp, memoria.hpp.
// ============================================================================

#pragma once

#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "cubo_semantico.hpp"
#include "hash.hpp"
#include "memoria.hpp"

// Error semantico
class ErrorSemantico : public std::runtime_error {
public:
    ErrorSemantico(const std::string& mensaje, int linea)
        : std::runtime_error(mensaje), linea(linea) {}
    int linea;
};

struct Variable {
    std::string nombre;
    Tipo tipo;
    int linea; // donde se declaro
    int direccion; // direccion virtual
};

// Tabla de variables de un ambito (global o local)
class TablaVariables {
public:
    // Lanza ErrorSemantico si el nombre ya existe en el ambito
    void revisarNueva(const std::string& nombre, int linea) const {
        if (contiene(nombre)) {
            throw ErrorSemantico("variable '" + nombre + "' declarada dos veces (antes en línea " +
                                     std::to_string(tabla.find(nombre)->linea) + ")",
                                 linea);
        }
    }

    // Agrega una variable con su direccion; lanza ErrorSemantico si ya existe en el ambito.
    void agregar(const std::string& nombre, Tipo tipo, int linea, int direccion) {
        revisarNueva(nombre, linea);
        tabla.put(nombre, Variable{nombre, tipo, linea, direccion});
        orden.push_back(nombre);
    }

    // Regresa la variable o nullptr si no esta en el ambito
    const Variable* buscar(const std::string& nombre) const {
        return tabla.find(nombre);
    }

    bool contiene(const std::string& nombre) const { return tabla.contains(nombre); }
    std::size_t tamano() const { return tabla.size(); }

    // Variables en el orden en que se declararon (para imprimir)
    std::vector<const Variable*> enOrden() const {
        std::vector<const Variable*> res;
        for (const auto& n : orden) res.push_back(tabla.find(n));
        return res;
    }

private:
    Hash<std::string, Variable> tabla;
    std::vector<std::string> orden;
};

struct Funcion {
    std::string nombre;
    int linea = 0;
    std::vector<Tipo> parametros; // tipos en orden
    std::vector<int> dirParametros; // direccion local de cada parametro (destino de PARAM)
    TablaVariables variables; // parametros + variables locales
    int inicio = -1; // cuadruplo donde empieza su codigo (destino del GOSUB)
    bool esVoid = true;
    Tipo tipoRetorno = Tipo::ERROR; // solo vale si !esVoid
    int direccionRetorno = memoria::SIN_DIRECCION; // global donde RETURN deja el valor
    bool tieneReturn = false; // si su cuerpo tiene al menos un return
    Recursos locales; // parametros + variables locales (segmento local)
    Recursos temporales; // temporales de su codigo (segmento temporal)
};

class DirectorioFunciones {
public:
    // Programa y variables globales
    void iniciarPrograma(const std::string& nombre) {
        programa = nombre;
        funcionActual = nullptr;
    }

    // Variable global (sección `var` antes de las funciones).
    void agregarGlobal(const std::string& nombre, Tipo tipo, int linea) {
        if (nombre == programa) {
            throw ErrorSemantico("'" + nombre + "' ya es el nombre del programa", linea);
        }
        if (funciones.contains(nombre)) {
            throw ErrorSemantico("'" + nombre + "' ya es el nombre de una función", linea);
        }
        globales.revisarNueva(nombre, linea);
        globales.agregar(nombre, tipo, linea, direccion(linea, [&] { return memoria.global(tipo); }));
        recursosGlobales.contar(tipo);
    }

    // Funciones

    // Da de alta una funcion y la deja como ambito actual
    Funcion& agregarFuncion(const std::string& nombre, int linea) {
        if (nombre == programa) {
            throw ErrorSemantico("'" + nombre + "' ya es el nombre del programa", linea);
        }
        if (funciones.contains(nombre)) {
            throw ErrorSemantico("función '" + nombre + "' declarada dos veces (antes en línea " +
                                     std::to_string(funciones.find(nombre)->linea) + ")",
                                 linea);
        }
        if (globales.contiene(nombre)) {
            throw ErrorSemantico("'" + nombre + "' ya es el nombre de una variable global", linea);
        }
        funciones.put(nombre, Funcion{});
        Funcion& f = *funciones.find(nombre);
        f.nombre = nombre;
        f.linea = linea;
        orden.push_back(nombre);
        funcionActual = &f;
        memoria.nuevoAmbito(); // sus locales y temporales empiezan desde el inicio del segmento
        return f;
    }

    // Parsmetro de la funcion actual va a su firma y a su tabla de variables
    void agregarParametro(const std::string& nombre, Tipo tipo, int linea) {
        agregarLocal(nombre, tipo, linea);
        funcionActual->parametros.push_back(tipo);
        funcionActual->dirParametros.push_back(funcionActual->variables.buscar(nombre)->direccion);
    }

    // Variable local de la funcion actual
    void agregarLocal(const std::string& nombre, Tipo tipo, int linea) {
        Funcion& f = actual(linea);
        f.variables.revisarNueva(nombre, linea);
        f.variables.agregar(nombre, tipo, linea, direccion(linea, [&] { return memoria.local(tipo); }));
        f.locales.contar(tipo);
    }

    // Tipo de retorno de la funcion actual (no se llama para funciones void); su valor de regreso vive en una direccion global
    void fijarRetorno(Tipo tipo, int linea) {
        if (!funcionActual) return;
        funcionActual->esVoid = false;
        funcionActual->tipoRetorno = tipo;
        funcionActual->direccionRetorno = direccion(linea, [&] { return memoria.global(tipo); });
        recursosGlobales.contar(tipo);
    }

    // Temporal del ambito actual (funcion o main); cuenta para su tamaño
    int temporal(Tipo tipo, int linea) {
        int dir = direccion(linea, [&] { return memoria.temporal(tipo); });
        (funcionActual ? funcionActual->temporales : temporalesMain).contar(tipo);
        return dir;
    }

    // Direccion de una constante (int/float) o de un letrero
    int constante(const std::string& valor, Tipo tipo, int linea) {
        return direccion(linea, [&] { return memoria.constante(valor, tipo); });
    }
    int letrero(const std::string& texto, int linea) {
        return direccion(linea, [&] { return memoria.letrero(texto); });
    }

    // Marca que la funcion actual tiene un return
    void marcarReturn() {
        if (funcionActual) funcionActual->tieneReturn = true;
    }

    // Cuadruplo donde empieza el codigo de la funcion actual
    void fijarInicio(int cuadruplo) {
        if (funcionActual) funcionActual->inicio = cuadruplo;
    }

    // Al terminar una funcion (o al llegar a main) se regresa al ambito global;
    // main empieza sus temporales desde el inicio del segmento
    void terminarFuncion() {
        funcionActual = nullptr;
        memoria.nuevoAmbito();
    }

    const Funcion* buscarFuncion(const std::string& nombre) const {
        return funciones.find(nombre);
    }

    // Busqueda de variables

    // Busca primero en la funcion actual y luego en las globales, regresa nullptr si no existe
    const Variable* buscarVariable(const std::string& nombre) const {
        if (funcionActual) {
            if (const Variable* v = funcionActual->variables.buscar(nombre)) return v;
        }
        return globales.buscar(nombre);
    }

    // Igual que buscarVariable, pero lanza error si no esta declarada
    const Variable& variable(const std::string& nombre, int linea) const {
        const Variable* v = buscarVariable(nombre);
        if (!v) throw ErrorSemantico("variable '" + nombre + "' no declarada", linea);
        return *v;
    }

    const std::string& nombrePrograma() const { return programa; }
    const TablaVariables& variablesGlobales() const { return globales; }
    const Funcion* enFuncion() const { return funcionActual; }
    const MemoriaVirtual& memoriaVirtual() const { return memoria; }
    const Recursos& recursosGlobal() const { return recursosGlobales; }
    const Recursos& recursosMain() const { return temporalesMain; }

    // Imprime el directorio completo
    void imprimir(std::ostream& os) const {
        os << "Programa: " << programa << "\n";
        os << "  Globales (" << recursosGlobales << "):\n";
        imprimirTabla(os, globales, "    ");
        for (const auto& n : orden) {
            const Funcion& f = *funciones.find(n);
            if (!f.esVoid) {
                os << "    " << f.nombre << " (regreso) : " << nombreTipo(f.tipoRetorno) << "  -> "
                   << f.direccionRetorno << "\n";
            }
        }
        os << "  main: temporales (" << temporalesMain << ")\n";
        for (const auto& n : orden) {
            const Funcion& f = *funciones.find(n);
            os << "  Función " << (f.esVoid ? "void" : nombreTipo(f.tipoRetorno)) << " "
               << f.nombre << "(";
            for (std::size_t i = 0; i < f.parametros.size(); i++) {
                os << (i ? ", " : "") << nombreTipo(f.parametros[i]);
            }
            os << ")  [línea " << f.linea << ", inicia en cuádruplo " << f.inicio << "]\n";
            os << "    locales (" << f.locales << "), temporales (" << f.temporales << ")\n";
            imprimirTabla(os, f.variables, "    ");
        }
        os << "  Constantes:\n";
        memoria.imprimirConstantes(os, "    ");
    }

private:
    Funcion& actual(int linea) {
        if (!funcionActual) throw ErrorSemantico("no hay una función abierta", linea);
        return *funcionActual;
    }

    static void imprimirTabla(std::ostream& os, const TablaVariables& t, const std::string& sangria) {
        if (t.tamano() == 0) os << sangria << "(vacía)\n";
        for (const Variable* v : t.enOrden()) {
            os << sangria << v->nombre << " : " << nombreTipo(v->tipo) << "  -> " << v->direccion
               << "  [línea " << v->linea << "]\n";
        }
    }

    // Pide una direccion a la memoria; si se acabo el segmento es un error semantico
    template <typename F>
    static int direccion(int linea, F pedir) {
        try {
            return pedir();
        } catch (const ErrorMemoria& e) {
            throw ErrorSemantico(e.what(), linea);
        }
    }

    std::string programa;
    TablaVariables globales;
    MemoriaVirtual memoria;
    Recursos recursosGlobales; // variables globales + valores de regreso
    Recursos temporalesMain;
    Hash<std::string, Funcion> funciones;
    std::vector<std::string> orden; // funciones en orden de declaración
    Funcion* funcionActual = nullptr;
};
