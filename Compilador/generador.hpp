// Traducción a cuádruplos de expresiones (aritméticas y relacionales), estatutos
// lineales (asignación, read, print), estatutos de control (if, while) y llamadas
// a función.
//
// Hereda del AnalizadorSemantico: el mismo recorrido llena el directorio de
// funciones y, con él, sabe el tipo de cada variable en el ámbito actual.
//
// Algoritmo (pilas de cuadruplos.hpp, sobre Stack/Queue de la Tarea 1):
//   - Cada operando (variable, constante o temporal) entra a PilaO y su tipo a PTipos.
//   - Cada operador entra a POper al leerlo.
//   - Al terminar el operando derecho de un operador se sacan dos operandos y el
//     operador, se revisa el tipo con el cubo semántico, se genera
//     (op, izq, der, tN) y el temporal tN entra a PilaO.
//   - '(' mete un fondo falso a POper para que los operadores de afuera esperen
//     a que termine la subexpresión; ')' lo saca.
//
// Precedencia: * / se resuelven al salir de cada factor, + - al salir de cada
// término y > < al salir de la expresión, así que * / quedan más "adentro".
// Como cada operador se resuelve al terminar su operando derecho, son asociativos
// a la izquierda: a - b - c = (a - b) - c.
//
// Control: GOTOF/GOTO se generan con el destino vacío y su posición se guarda en
// PSaltos; cuando se llega al destino se "rellena" (backpatching).
// Llamadas: ERA (reserva espacio), PARAM por argumento, GOSUB al inicio de la
// función; cada función termina con ENDFUNC. Una función con tipo regresa con
// RETURN, que deja el valor en una variable global con su nombre; si la llamada
// está en una expresión, después del GOSUB se copia ese valor a un temporal. El programa empieza con un GOTO a main
// y termina con END.
//
// Los temporales se llaman _t1, _t2, ... (un ID de BabyDuck no puede empezar con
// '_', así que no chocan con variables del programa).

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
    //   llamada con tipo: (=, funcion, _, tN) copia el valor regresado a un temporal;
    //   llamada void: error, no regresa valor
    // y despues, si arriba de POper hay * o /, se genera ese cuadruplo.
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
                std::string temp = nuevoTemporal();
                codigo.generar("=", f->nombre, "", temp);
                codigo.pushOperando(temp, f->tipoRetorno);
            } else {
                if (f) {
                    intentar(id, [&] {
                        throw ErrorSemantico("la función '" + id->getText() +
                                                 "' es void y no regresa un valor",
                                             linea(id));
                    });
                }
                codigo.pushOperando(id ? id->getText() : "?", Tipo::ERROR);
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
        codigo.generar(simboloOp(Op::ASIGNA), valor, "", v->nombre);
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
        codigo.generar("RETURN", valor, "", f->nombre);
    }

    // PN-L2  read ( ID ) ; genera (read, _, _, ID)
    void exitRead(BabyDuckParser::ReadContext* ctx) override {
        auto* id = ctx->ID();
        if (omitirFuncion || !id || !directorio.buscarVariable(id->getText())) return;
        codigo.generar("read", "", "", id->getText());
    }

    // PN-L3  cada elemento de print genera (print, _, _, valor); un letrero va tal cual, con comillas
    void exitPrintItem(BabyDuckParser::PrintItemContext* ctx) override {
        if (omitirFuncion) return;
        if (ctx->CTE_STRING()) {
            codigo.generar("print", "", "", ctx->CTE_STRING()->getText());
        } else if (!codigo.pilaO.empty()) {
            codigo.generar("print", "", "", codigo.popOperando().first);
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

    // PN-F2  despues de cada argumento: (PARAM, valor, _, parK) y se revisa su tipo
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
    int temporales = 0;

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
        return codigo.generar("GOTOF", cond, "", "");
    }

    // Saca el argumento de PilaO, revisa su tipo contra el parametro y genera PARAM
    void argumento(BabyDuckParser::ExpresionContext* ctx) {
        auto& [f, k] = llamadas.top();
        auto [valor, tipo] = codigo.popOperando();
        Tipo t = tipo;
        std::size_t num = ++k;
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
        codigo.generar("PARAM", valor, "", "par" + std::to_string(num));
    }

    std::string nuevoTemporal() { return "_t" + std::to_string(++temporales); }

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
        std::string temp = nuevoTemporal();
        codigo.generar(simboloOp(op), izq, der, temp);
        codigo.pushOperando(temp, t);
    }

    static bool negativo(BabyDuckParser::SignOptContext* signo) {
        return signo && signo->MINUS();
    }

    // ID con signo opcional; -x genera (NEG, x, _, tN)
    void operandoVariable(Nodo* id, BabyDuckParser::SignOptContext* signo) {
        const Variable* v = directorio.buscarVariable(id->getText());
        Tipo tipo = v ? v->tipo : Tipo::ERROR; // no declarada: ya se reporto
        if (!negativo(signo)) {
            codigo.pushOperando(id->getText(), tipo);
            return;
        }
        std::string temp = nuevoTemporal();
        codigo.generar("NEG", id->getText(), "", temp);
        codigo.pushOperando(temp, resultadoUnario(tipo));
    }

    // Una constante con signo es una sola constante: -3.5
    void operandoConstante(BabyDuckParser::CteContext* cte, BabyDuckParser::SignOptContext* signo) {
        Tipo tipo = cte->CTE_FLOAT() ? Tipo::FLOAT : Tipo::INT;
        codigo.pushOperando((negativo(signo) ? "-" : "") + cte->getText(), tipo);
    }
};
