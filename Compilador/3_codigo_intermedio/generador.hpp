// ============================================================================
// generador.hpp
// Fase 4: código intermedio (incluye la fase 3 por herencia)
//
//   texto ─► scanner ─► tokens ─► parser ─► árbol ─► SEMÁNTICA ─► CUÁDRUPLOS
//
// Qué hace:
//   Clase GeneradorCuadruplos: los puntos neurálgicos que traducen el programa a
//   cuádruplos: expresiones, asignación/read/print, if/while (con saltos que se
//   rellenan después) y llamadas a función. También revisa tipos con el cubo.
//
// Papel en el proceso:
//   Es la clase con la que main.cpp recorre el árbol. Hereda de
//   AnalizadorSemantico, así que un solo recorrido hace la semántica y la
//   traducción.
//   Recibe: el árbol del parser.  Entrega: codigo.cuadruplos (la fila final) y
//   el directorio lleno.
//   Usa: cuadruplos.hpp, semantica.hpp (y por ellos todo lo demás).
// ============================================================================

#pragma once

#include <string>

#include "cuadruplos.hpp"
#include "semantica.hpp"

class GeneradorCuadruplos : public AnalizadorSemantico {
public:
    EstructurasCodigo codigo;

    // Operadores: se meten a POper al leerlos

    // PN-E1  + -
    void enterExpArTail(BabyDuckParser::ExpArTailContext* ctx) override {
        if (ctx->PLUS()) pushOperador(Op::SUMA, ctx->PLUS());
        if (ctx->MINUS()) pushOperador(Op::RESTA, ctx->MINUS());
    }

    // PN-E2  * /
    void enterTermTail(BabyDuckParser::TermTailContext* ctx) override {
        if (ctx->TIMES()) pushOperador(Op::MULT, ctx->TIMES());
        if (ctx->DIVIDE()) pushOperador(Op::DIV, ctx->DIVIDE());
    }

    // PN-E3  > <
    void enterExpRelTail(BabyDuckParser::ExpRelTailContext* ctx) override {
        if (ctx->GT()) pushOperador(Op::MAYOR, ctx->GT());
        if (ctx->LT()) pushOperador(Op::MENOR, ctx->LT());
    }

    // Factores

    // PN-E4  '(' fondo falso
    void enterFactor(BabyDuckParser::FactorContext* ctx) override {
        AnalizadorSemantico::enterFactor(ctx); // revisa que la variable exista
        if (omitirFuncion) return;
        if (ctx->LPAREN()) pushOperador(Op::FONDO_FALSO, ctx->LPAREN());
    }

    // PN-E5  al terminar un factor:
    //   ( expresion ) se quita el fondo falso (el resultado ya esta en PilaO)
    //   ID / cte se mete el operando (con su signo)
    //   llamada con tipo: (=, dirRegreso, _, tN) copia el valor regresado a un temporal;
    //   llamada void: error, no regresa valor y despues, si arriba de POper hay * o /, se genera ese cuadruplo
    void exitFactor(BabyDuckParser::FactorContext* ctx) override {
        if (omitirFuncion) return;
        if (ctx->LPAREN()) {
            popOperador(); // fondo falso
        } else if (ctx->ID()) {
            operandoVariable(ctx->ID(), ctx->signOpt());
        } else if (ctx->cte()) {
            operandoConstante(ctx->cte(), ctx->signOpt());
        } else if (auto* llamada = ctx->llamada()) {
            auto* id = llamada->ID();
            const Funcion* f = id ? directorio.buscarFuncion(id->getText()) : nullptr;
            if (f && !f->esVoid) {
                // Justo despues del GOSUB: el valor se copia antes de que otra llamada lo pise
                int temp = nuevoTemporal(f->tipoRetorno, id);
                codigo.generar("=", dir(f->direccionRetorno), "", dir(temp));
                codigo.pushOperando(temp, f->tipoRetorno);
            } else {
                if (f) {
                    intentar(id, [&] {
                        throw ErrorSemantico("la función '" + id->getText() +
                                                 "' es void y no regresa un valor",
                                             linea(id));
                    });
                }
                codigo.pushOperando(memoria::SIN_DIRECCION, Tipo::ERROR);
            }
        }
        resolverSi(Op::MULT, Op::DIV);
    }

    // PN-E6 al terminar un termino: si arriba de POper hay + o -, se genera
    void exitTermino(BabyDuckParser::TerminoContext*) override {
        if (omitirFuncion) return;
        resolverSi(Op::SUMA, Op::RESTA);
    }

    // Estatutos lineales

    // PN-L1  ID = expresion ;   genera (=, valor, _, ID)
    void exitAssign(BabyDuckParser::AssignContext* ctx) override {
        auto* id = ctx->ID();
        if (omitirFuncion || !id || codigo.pilaO.empty()) return;
        auto [valor, tipo] = codigo.popOperando();
        Tipo tValor = tipo; // (una lambda no puede capturar un structured binding en C++17)
        const Variable* v = directorio.buscarVariable(id->getText());
        if (!v) return; // ya se reporta como no declarada
        if (resultado(v->tipo, tValor, Op::ASIGNA) == Tipo::ERROR && tValor != Tipo::ERROR) {
            intentar(ctx->ASSIGN(), [&] {
                throw ErrorSemantico("no se puede asignar un " + nombreTipo(tValor) + " a '" +
                                         v->nombre + "' (" + nombreTipo(v->tipo) + ")",
                                     linea(ctx->ASSIGN()));
            });
        }
        codigo.generar(simboloOp(Op::ASIGNA), dir(valor), "", dir(v->direccion));
    }

    // PN-R1  return expresion ; : revisa el tipo contra el de la funcion y genera
    //        (RETURN, valor, _, funcion): guarda el valor y termina la funcion
    void exitRetorno(BabyDuckParser::RetornoContext* ctx) override {
        if (omitirFuncion || codigo.pilaO.empty()) return;
        auto [valor, tipo] = codigo.popOperando();
        Tipo t = tipo;
        const Funcion* f = directorio.enFuncion();
        if (!f || f->esVoid) return; // ya se reporto en PN9
        if (t != Tipo::ERROR && resultado(f->tipoRetorno, t, Op::ASIGNA) == Tipo::ERROR) {
            intentar(ctx->RETURN(), [&] {
                throw ErrorSemantico("la función '" + f->nombre + "' es " +
                                         nombreTipo(f->tipoRetorno) + " y regresa un " + nombreTipo(t),
                                     linea(ctx->RETURN()));
            });
        }
        codigo.generar("RETURN", dir(valor), "", dir(f->direccionRetorno));
    }

    // PN-L2  read ( ID ) ; genera (read, _, _, ID)
    void exitRead(BabyDuckParser::ReadContext* ctx) override {
        auto* id = ctx->ID();
        if (omitirFuncion || !id) return;
        const Variable* v = directorio.buscarVariable(id->getText());
        if (v) codigo.generar("read", "", "", dir(v->direccion));
    }

    // PN-L3  cada elemento de print genera (print, _, _, valor); un letrero es una
    //        constante string con su propia direccion
    void exitPrintItem(BabyDuckParser::PrintItemContext* ctx) override {
        if (omitirFuncion) return;
        if (auto* s = ctx->CTE_STRING()) {
            int d = memoria::SIN_DIRECCION;
            intentar(s, [&] { d = directorio.letrero(s->getText(), linea(s)); });
            codigo.generar("print", "", "", dir(d));
        } else if (!codigo.pilaO.empty()) {
            codigo.generar("print", "", "", dir(codigo.popOperando().first));
        }
    }

    // Estatutos no lineales: if / while

    // PN-C1  despues de la condicion de if o while: (GOTOF, cond, _, ?)
    //        el salto queda pendiente en PSaltos hasta saber a donde va
    void enterBody(BabyDuckParser::BodyContext* ctx) override {
        AnalizadorSemantico::enterBody(ctx);
        if (omitirFuncion) return;
        if (auto* c = dynamic_cast<BabyDuckParser::ConditionContext*>(ctx->parent)) {
            codigo.psaltos.push(gotoFalso(c->IF()));
        } else if (auto* w = dynamic_cast<BabyDuckParser::CycleContext*>(ctx->parent)) {
            codigo.psaltos.push(gotoFalso(w->WHILE()));
        } else if (dynamic_cast<BabyDuckParser::ProgramaContext*>(ctx->parent)) {
            codigo.rellenar(saltoMain, codigo.siguiente()); // PN-P2 aqui empieza main
        }
    }

    // PN-C2  else: (GOTO, _, _, ?) para saltar el else al terminar el if;
    //        el GOTOF del if se rellena para que caiga al inicio del else
    void enterElseopt(BabyDuckParser::ElseoptContext* ctx) override {
        if (omitirFuncion || !ctx->ELSE()) return;
        std::size_t falso = sacar(codigo.psaltos);
        codigo.psaltos.push(codigo.generar("GOTO", "", "", ""));
        codigo.rellenar(falso, codigo.siguiente());
    }

    // PN-C3  fin del if: el salto pendiente (GOTOF sin else, o GOTO del else) va aqui
    void exitCondition(BabyDuckParser::ConditionContext*) override {
        if (omitirFuncion) return;
        codigo.rellenar(sacar(codigo.psaltos), codigo.siguiente());
    }

    // PN-C4  inicio del while: se guarda a donde regresar para volver a evaluar la condicion
    void enterCycle(BabyDuckParser::CycleContext*) override {
        if (omitirFuncion) return;
        codigo.psaltos.push(codigo.siguiente());
    }

    // PN-C5  fin del while: (GOTO, _, _, regreso) y el GOTOF sale aqui
    void exitCycle(BabyDuckParser::CycleContext*) override {
        if (omitirFuncion) return;
        std::size_t falso = sacar(codigo.psaltos);
        std::size_t regreso = sacar(codigo.psaltos);
        codigo.generar("GOTO", "", "", std::to_string(regreso));
        codigo.rellenar(falso, codigo.siguiente());
    }

    // Llamadas a funcion

    // PN-F1  ID ( : (ERA, _, _, funcion) reserva su espacio; los argumentos
    //        son subexpresiones, van sobre un fondo falso
    void enterLlamada(BabyDuckParser::LlamadaContext* ctx) override {
        AnalizadorSemantico::enterLlamada(ctx); // existe y numero de argumentos
        if (omitirFuncion) return;
        const Funcion* f = ctx->ID() ? directorio.buscarFuncion(ctx->ID()->getText()) : nullptr;
        if (f) codigo.generar("ERA", "", "", f->nombre);
        llamadas.push({f, 0});
        pushOperador(Op::FONDO_FALSO, ctx->LPAREN());
    }

    // PN-F2  despues de cada argumento: (PARAM, valor, _, dirParametroK) y se revisa su tipo
    // PN-E7  al terminar una expresion: si arriba de POper hay > o <, se genera
    void exitExpresion(BabyDuckParser::ExpresionContext* ctx) override {
        if (omitirFuncion) return;
        resolverSi(Op::MAYOR, Op::MENOR);
        bool esArgumento = dynamic_cast<BabyDuckParser::ArglistContext*>(ctx->parent) ||
                           dynamic_cast<BabyDuckParser::ArglistTailContext*>(ctx->parent);
        if (esArgumento && !llamadas.empty()) argumento(ctx);
    }

    // PN-F3  ) : (GOSUB, funcion, _, inicio) salta al codigo de la funcion
    void exitLlamada(BabyDuckParser::LlamadaContext*) override {
        if (omitirFuncion) return;
        popOperador(); // fondo falso
        const Funcion* f = sacar(llamadas).first;
        if (f) codigo.generar("GOSUB", f->nombre, "", std::to_string(f->inicio));
    }

    // Programa y funciones

    // PN-P1  program ID : (GOTO, _, _, ?) salta las funciones hasta main
    void enterPrograma(BabyDuckParser::ProgramaContext* ctx) override {
        AnalizadorSemantico::enterPrograma(ctx);
        saltoMain = codigo.generar("GOTO", "", "", "");
    }

    // PN-P3  end : (END, _, _, _)
    void exitPrograma(BabyDuckParser::ProgramaContext*) override {
        codigo.generar("END", "", "", "");
    }

    // PN-F4  void ID : su codigo empieza en el siguiente cuadruplo
    void enterFuncs(BabyDuckParser::FuncsContext* ctx) override {
        AnalizadorSemantico::enterFuncs(ctx);
        if (!omitirFuncion) directorio.fijarInicio(static_cast<int>(codigo.siguiente()));
    }

    // PN-F5  ] ; : (ENDFUNC, _, _, _) regresa a quien la llamo
    void exitFuncs(BabyDuckParser::FuncsContext* ctx) override {
        if (!omitirFuncion) codigo.generar("ENDFUNC", "", "", "");
        AnalizadorSemantico::exitFuncs(ctx);
    }

private:
    Stack<Nodo*> tokensOp; // token de cada operador de POper (para ubicar errores)
    // Llamadas abiertas (pueden anidarse en los argumentos): funcion y argumentos vistos
    Stack<std::pair<const Funcion*, std::size_t>> llamadas;
    std::size_t saltoMain = 0; // GOTO inicial, se rellena al llegar a main

    // Saca la condicion de PilaO, revisa que sea bool y genera (GOTOF, cond, _, ?)
    std::size_t gotoFalso(Nodo* donde) {
        auto [cond, tipo] = codigo.popOperando();
        Tipo t = tipo;
        if (t != Tipo::BOOL && t != Tipo::ERROR) {
            intentar(donde, [&] {
                throw ErrorSemantico("la condición debe ser bool y es " + nombreTipo(t),
                                     linea(donde));
            });
        }
        return codigo.generar("GOTOF", dir(cond), "", "");
    }

    // Saca el argumento de PilaO, revisa su tipo contra el parametro y genera PARAM
    void argumento(BabyDuckParser::ExpresionContext* ctx) {
        auto& [f, k] = llamadas.top();
        auto [valor, tipo] = codigo.popOperando();
        Tipo t = tipo;
        std::size_t num = ++k;
        int destino = memoria::SIN_DIRECCION;
        if (f && num <= f->dirParametros.size()) destino = f->dirParametros[num - 1];
        if (f && num <= f->parametros.size() && t != Tipo::ERROR) {
            Tipo esperado = f->parametros[num - 1];
            if (resultado(esperado, t, Op::ASIGNA) == Tipo::ERROR) {
                std::string nombre = f->nombre;
                auto* token = ctx->getStart();
                ErrorSemantico e("el argumento " + std::to_string(num) + " de '" + nombre +
                                     "' debe ser " + nombreTipo(esperado) + " y es " +
                                     nombreTipo(t),
                                 static_cast<int>(token->getLine()));
                std::cerr << "Error semántico [línea " << e.linea << ", columna "
                          << token->getCharPositionInLine() + 1 << "]: " << e.what() << "\n";
                errores++;
            }
        }
        codigo.generar("PARAM", dir(valor), "", dir(destino));
    }

    static std::string dir(int direccion) { return std::to_string(direccion); }

    // Pide un temporal del tipo dado al ambito actual. Un resultado con error de tipo no ocupa memoria (el programa se rechaza y no se imprimen cuadruplos).
    int nuevoTemporal(Tipo tipo, Nodo* donde) {
        int d = memoria::SIN_DIRECCION;
        if (tipo == Tipo::ERROR) return d;
        intentar(donde, [&] { d = directorio.temporal(tipo, linea(donde)); });
        return d;
    }

    void pushOperador(Op op, Nodo* token) {
        codigo.poper.push(op);
        tokensOp.push(token);
    }

    Op popOperador() {
        tokensOp.pop();
        return sacar(codigo.poper);
    }

    // Si el tope de POper es op1 u op2, genera su cuadruplo
    void resolverSi(Op op1, Op op2) {
        if (codigo.poper.empty()) return;
        Op op = codigo.poper.top();
        if (op != op1 && op != op2) return;
        Nodo* token = tokensOp.top();
        popOperador();
        auto [der, tDer] = codigo.popOperando();
        auto [izq, tIzq] = codigo.popOperando();
        Tipo ti = tIzq, td = tDer;
        Tipo t = resultado(ti, td, op);
        // Si algun operando ya era ERROR el problema ya se reporto
        if (t == Tipo::ERROR && tIzq != Tipo::ERROR && tDer != Tipo::ERROR) {
            intentar(token, [&] {
                throw ErrorSemantico("tipos incompatibles: " + nombreTipo(ti) + " " +
                                         simboloOp(op) + " " + nombreTipo(td),
                                     linea(token));
            });
        }
        int temp = nuevoTemporal(t, token);
        codigo.generar(simboloOp(op), dir(izq), dir(der), dir(temp));
        codigo.pushOperando(temp, t);
    }

    static bool negativo(BabyDuckParser::SignOptContext* signo) {
        return signo && signo->MINUS();
    }

    // ID con signo opcional: entra su direccion; -x genera (NEG, x, _, tN)
    void operandoVariable(Nodo* id, BabyDuckParser::SignOptContext* signo) {
        const Variable* v = directorio.buscarVariable(id->getText());
        Tipo tipo = v ? v->tipo : Tipo::ERROR; // no declarada: ya se reporto
        int d = v ? v->direccion : memoria::SIN_DIRECCION;
        if (!negativo(signo)) {
            codigo.pushOperando(d, tipo);
            return;
        }
        Tipo t = resultadoUnario(tipo);
        int temp = nuevoTemporal(t, id);
        codigo.generar("NEG", dir(d), "", dir(temp));
        codigo.pushOperando(temp, t);
    }

    // Una constante con signo es una sola constante (-3.5); entra su direccion de la tabla de constantes
    void operandoConstante(BabyDuckParser::CteContext* cte, BabyDuckParser::SignOptContext* signo) {
        Nodo* token = cte->CTE_FLOAT() ? cte->CTE_FLOAT() : cte->CTE_INT();
        Tipo tipo = cte->CTE_FLOAT() ? Tipo::FLOAT : Tipo::INT;
        std::string valor = (negativo(signo) ? "-" : "") + cte->getText();
        int d = memoria::SIN_DIRECCION;
        intentar(token, [&] { d = directorio.constante(valor, tipo, linea(token)); });
        codigo.pushOperando(d, tipo);
    }
};
