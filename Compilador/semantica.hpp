// Puntos neurálgicos para crear y llenar el Directorio de Funciones y las
// Tablas de Variables.
//
// Se implementan como un listener de ANTLR: después de que el parser arma el
// árbol, ParseTreeWalker lo recorre y llama enterX / exitX al entrar y salir de
// cada regla. Cada método marcado con "PN" es un punto neurálgico.
//
// Los errores no detienen el análisis: se reportan con línea y columna y se
// sigue con el resto del programa para encontrar todos los errores de una vez.

#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "BabyDuckBaseListener.h"
#include "BabyDuckParser.h"
#include "tablas.hpp"

class AnalizadorSemantico : public BabyDuckBaseListener {
public:
    DirectorioFunciones directorio;
    int errores = 0;

    // PN1 program ID: se crea el directorio con el nombre del programa
    void enterPrograma(BabyDuckParser::ProgramaContext* ctx) override {
        if (ctx->ID()) directorio.iniciarPrograma(ctx->ID()->getText());
    }

    // PN2 var idlist : type ; cada ID va a la tabla del ambito actual
    void exitVars(BabyDuckParser::VarsContext* ctx) override {
        if (omitirFuncion || !ctx->type()) return;
        Tipo tipo = tipoDe(ctx->type());
        for (auto* id : idsDe(ctx->idlist())) {
            intentar(id, [&] {
                if (directorio.enFuncion()) {
                    directorio.agregarLocal(id->getText(), tipo, linea(id));
                } else {
                    directorio.agregarGlobal(id->getText(), tipo, linea(id));
                }
            });
        }
    }

    // PN3 (void | type) ID: se da de alta la funcion con su tipo de retorno y se abre su tabla de variables
    void enterFuncs(BabyDuckParser::FuncsContext* ctx) override {
        auto* id = ctx->ID();
        if (!id) return;
        bool ok = intentar(id, [&] { directorio.agregarFuncion(id->getText(), linea(id)); });
        // Si la funcion esta repetida no se analiza su cuerpo, para no mezclar sus variables con las de la primera ni generar errores en cascada
        omitirFuncion = !ok;
        if (ok && ctx->type()) directorio.fijarRetorno(tipoDe(ctx->type()));
    }

    // PN4 ID : type  (cada parametro) lleva a la firma y a la tabla local
    void enterParams(BabyDuckParser::ParamsContext* ctx) override {
        agregarParametro(ctx->ID(), ctx->type());
    }
    void enterParamsTail(BabyDuckParser::ParamsTailContext* ctx) override {
        agregarParametro(ctx->ID(), ctx->type());
    }

    // PN5: ] ; (fin de la funcion), una funcion con tipo debe tener un return;
    //      se cierra su ambito, se regresa a globales
    void exitFuncs(BabyDuckParser::FuncsContext* ctx) override {
        const Funcion* f = directorio.enFuncion();
        if (!omitirFuncion && f && !f->esVoid && !f->tieneReturn && ctx->ID()) {
            intentar(ctx->ID(), [&] {
                throw ErrorSemantico("la función '" + f->nombre + "' es " +
                                         nombreTipo(f->tipoRetorno) + " y no tiene return",
                                     linea(ctx->ID()));
            });
        }
        directorio.terminarFuncion();
        omitirFuncion = false;
    }

    // PN9 return: solo dentro de una funcion que no sea void
    void enterRetorno(BabyDuckParser::RetornoContext* ctx) override {
        auto* r = ctx->RETURN();
        if (omitirFuncion || !r) return;
        const Funcion* f = directorio.enFuncion();
        intentar(r, [&] {
            if (!f) throw ErrorSemantico("return fuera de una función", linea(r));
            if (f->esVoid) {
                throw ErrorSemantico("la función '" + f->nombre + "' es void y no puede regresar un valor",
                                     linea(r));
            }
        });
        directorio.marcarReturn();
    }

    // PN6 main el cuerpo de main usa el ambito global
    void enterBody(BabyDuckParser::BodyContext* ctx) override {
        if (dynamic_cast<BabyDuckParser::ProgramaContext*>(ctx->parent)) {
            directorio.terminarFuncion();
        }
    }

    // PN7 uso de una variable (asignacion, factor y read) debe estar declarada
    void enterAssign(BabyDuckParser::AssignContext* ctx) override { usarVariable(ctx->ID()); }
    void enterFactor(BabyDuckParser::FactorContext* ctx) override { usarVariable(ctx->ID()); }
    void enterRead(BabyDuckParser::ReadContext* ctx) override { usarVariable(ctx->ID()); }

    // PN8 llamada ID (args) a la funcion debe existir y recibir tantos argumentos como parametros tiene
    void enterLlamada(BabyDuckParser::LlamadaContext* ctx) override {
        auto* id = ctx->ID();
        if (omitirFuncion || !id) return;
        intentar(id, [&] {
            const Funcion* f = directorio.buscarFuncion(id->getText());
            if (!f) throw ErrorSemantico("función '" + id->getText() + "' no declarada", linea(id));
            std::size_t n = numArgumentos(ctx->arglist());
            if (n != f->parametros.size()) {
                throw ErrorSemantico("'" + f->nombre + "' espera " +
                                         std::to_string(f->parametros.size()) +
                                         " argumento(s) y recibió " + std::to_string(n),
                                     linea(id));
            }
        });
    }

protected:
    bool omitirFuncion = false;

    using Nodo = antlr4::tree::TerminalNode;

    static int linea(Nodo* n) { return static_cast<int>(n->getSymbol()->getLine()); }
    static int columna(Nodo* n) {
        return static_cast<int>(n->getSymbol()->getCharPositionInLine()) + 1;
    }

    // Ejecuta una accion sobre las tablas, si lanza ErrorSemantico lo reporta
    template <typename F>
    bool intentar(Nodo* donde, F accion) {
        try {
            accion();
            return true;
        } catch (const ErrorSemantico& e) {
            std::cerr << "Error semántico [línea " << e.linea << ", columna " << columna(donde)
                      << "]: " << e.what() << "\n";
            errores++;
            return false;
        }
    }

    // arglist : expresion arglistTail | ε ;  arglistTail : , expresion arglistTail | ε
    static std::size_t numArgumentos(BabyDuckParser::ArglistContext* args) {
        if (!args || !args->expresion()) return 0;
        std::size_t n = 1;
        for (auto* t = args->arglistTail(); t && t->expresion(); t = t->arglistTail()) n++;
        return n;
    }

private:
    void agregarParametro(Nodo* id, BabyDuckParser::TypeContext* tipo) {
        if (omitirFuncion || !id || !tipo) return; // params vacio
        intentar(id, [&] { directorio.agregarParametro(id->getText(), tipoDe(tipo), linea(id)); });
    }

    void usarVariable(Nodo* id) {
        if (omitirFuncion || !id) return; // factor sin ID (constante, llamada o parentesis)
        intentar(id, [&] { directorio.variable(id->getText(), linea(id)); });
    }

    static Tipo tipoDe(BabyDuckParser::TypeContext* t) {
        return t->FLOAT() ? Tipo::FLOAT : Tipo::INT;
    }

    // idlist : ID idlistElem ;  idlistElem : , ID idlistElem | ε
    static std::vector<Nodo*> idsDe(BabyDuckParser::IdlistContext* lista) {
        std::vector<Nodo*> ids;
        if (!lista) return ids;
        if (lista->ID()) ids.push_back(lista->ID());
        for (auto* e = lista->idlistElem(); e && e->ID(); e = e->idlistElem()) {
            ids.push_back(e->ID());
        }
        return ids;
    }
};
