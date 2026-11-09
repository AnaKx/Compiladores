// Directorio de Funciones y Tablas de Variables de BabyDuck.
//
// Estructura:
//
//   DirectorioFunciones
//   ├── programa                nombre del programa
//   ├── globales : TablaVariables      (vars del programa, las usa main)
//   └── funciones : nombre -> Funcion
//         └── Funcion
//               ├── nombre, linea
//               ├── parametros : [Tipo]     (en orden, para revisar llamadas)
//               └── variables  : TablaVariables   (params + vars locales)
//
// Se usan tablas hash (std::unordered_map) porque la operación más frecuente es
// buscar un nombre (cada vez que aparece un ID), y la búsqueda es O(1) en promedio.
// Todas las funciones de BabyDuck son void, por eso Funcion no guarda tipo de retorno.

#pragma once

#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "cubo_semantico.hpp"

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
};

// Tabla de variables de un ambito (global o local)
class TablaVariables {
public:
    // Agrega una variable; lanza ErrorSemantico si ya existe en el ambito.
    void agregar(const std::string& nombre, Tipo tipo, int linea) {
        if (contiene(nombre)) {
            throw ErrorSemantico("variable '" + nombre + "' declarada dos veces (antes en línea " +
                                     std::to_string(tabla.at(nombre).linea) + ")",
                                 linea);
        }
        tabla.emplace(nombre, Variable{nombre, tipo, linea});
        orden.push_back(nombre);
    }

    // Regresa la variable o nullptr si no esta en el ambito
    const Variable* buscar(const std::string& nombre) const {
        auto it = tabla.find(nombre);
        return it == tabla.end() ? nullptr : &it->second;
    }

    bool contiene(const std::string& nombre) const { return tabla.count(nombre) > 0; }
    std::size_t tamano() const { return tabla.size(); }

    // Variables en el orden en que se declararon (para imprimir)
    std::vector<const Variable*> enOrden() const {
        std::vector<const Variable*> res;
        for (const auto& n : orden) res.push_back(&tabla.at(n));
        return res;
    }

private:
    std::unordered_map<std::string, Variable> tabla;
    std::vector<std::string> orden;
};

struct Funcion {
    std::string nombre;
    int linea = 0;
    std::vector<Tipo> parametros; // tipos en orden
    TablaVariables variables; // parámetros + variables locales
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
        if (funciones.count(nombre)) {
            throw ErrorSemantico("'" + nombre + "' ya es el nombre de una función", linea);
        }
        globales.agregar(nombre, tipo, linea);
    }

    // Funciones

    // Da de alta una funcion y la deja como ambito actual
    Funcion& agregarFuncion(const std::string& nombre, int linea) {
        if (nombre == programa) {
            throw ErrorSemantico("'" + nombre + "' ya es el nombre del programa", linea);
        }
        if (funciones.count(nombre)) {
            throw ErrorSemantico("función '" + nombre + "' declarada dos veces (antes en línea " +
                                     std::to_string(funciones.at(nombre).linea) + ")",
                                 linea);
        }
        if (globales.contiene(nombre)) {
            throw ErrorSemantico("'" + nombre + "' ya es el nombre de una variable global", linea);
        }
        Funcion& f = funciones[nombre];
        f.nombre = nombre;
        f.linea = linea;
        orden.push_back(nombre);
        funcionActual = &f;
        return f;
    }

    // Parsmetro de la funcion actual va a su firma y a su tabla de variables
    void agregarParametro(const std::string& nombre, Tipo tipo, int linea) {
        Funcion& f = actual(linea);
        f.variables.agregar(nombre, tipo, linea);
        f.parametros.push_back(tipo);
    }

    // Variable local de la funcion actual
    void agregarLocal(const std::string& nombre, Tipo tipo, int linea) {
        actual(linea).variables.agregar(nombre, tipo, linea);
    }

    // Al terminar una funcion (o al llegar a main) se regresa al ambito global
    void terminarFuncion() { funcionActual = nullptr; }

    const Funcion* buscarFuncion(const std::string& nombre) const {
        auto it = funciones.find(nombre);
        return it == funciones.end() ? nullptr : &it->second;
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

    // Imprime el directorio completo
    void imprimir(std::ostream& os) const {
        os << "Programa: " << programa << "\n";
        os << "  Globales:\n";
        imprimirTabla(os, globales, "    ");
        for (const auto& n : orden) {
            const Funcion& f = funciones.at(n);
            os << "  Función " << f.nombre << "(";
            for (std::size_t i = 0; i < f.parametros.size(); i++) {
                os << (i ? ", " : "") << nombreTipo(f.parametros[i]);
            }
            os << ")  [línea " << f.linea << "]\n";
            imprimirTabla(os, f.variables, "    ");
        }
    }

private:
    Funcion& actual(int linea) {
        if (!funcionActual) throw ErrorSemantico("no hay una función abierta", linea);
        return *funcionActual;
    }

    static void imprimirTabla(std::ostream& os, const TablaVariables& t, const std::string& sangria) {
        if (t.tamano() == 0) os << sangria << "(vacía)\n";
        for (const Variable* v : t.enOrden()) {
            os << sangria << v->nombre << " : " << nombreTipo(v->tipo) << "  [línea " << v->linea
               << "]\n";
        }
    }

    std::string programa;
    TablaVariables globales;
    std::unordered_map<std::string, Funcion> funciones;
    std::vector<std::string> orden; // funciones en orden de declaración
    Funcion* funcionActual = nullptr;
};
