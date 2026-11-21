// ============================================================================
// main.cpp
// Punto de entrada: dirige todas las fases
//
//   texto ─► SCANNER ─► tokens ─► PARSER ─► árbol ─► SEMÁNTICA ─► CUÁDRUPLOS
//
// Qué hace:
//   Lee las opciones y el archivo fuente y corre las fases en orden:
//   1) scanner (BabyDuckLexer), 2) parser (BabyDuckParser), y si no hubo errores
//   3-4) recorre el árbol con GeneradorCuadruplos (semántica + cuádruplos).
//   Junta los errores de todas las fases.
//
// Papel en el proceso:
//   Recibe: la ruta de un programa BabyDuck y opciones de impresión.
//   Entrega: "Programa aceptado" (regresa 0) o los errores y "Programa rechazado"
//   (regresa 1). Con --tokens, --tree, --tablas y --cuadruplos imprime el
//   resultado de cada fase.
//   Usa: el código que genera ANTLR (generated/) y generador.hpp, que incluye
//   todo lo demás.
// ============================================================================

#include <fstream>
#include <iostream>
#include <string>

#include "antlr4-runtime.h"
#include "BabyDuckLexer.h"
#include "BabyDuckParser.h"
#include "generador.hpp"

using namespace antlr4;

// Remplazo el reporte de errores de ANTLR porque no me agrada lol
// Se cuentan los errores e indica si son lexicos o sintacticos
class ReporteErrores : public BaseErrorListener {
public:
    explicit ReporteErrores(const std::string& tipo) : tipo(tipo) {}

    void syntaxError(Recognizer*, Token*, size_t linea, size_t columna,
                     const std::string& mensaje, std::exception_ptr) override {
        std::cerr << "Error " << tipo << " [línea " << linea << ", columna "
                  << columna + 1 << "]: " << mensaje << "\n";
        errores++;
    }

    std::string tipo;
    int errores = 0;
};

int main(int argc, char* argv[]) {
    bool mostrarTokens = false;
    bool mostrarArbol = false;
    bool mostrarTablas = false;
    bool mostrarCuadruplos = false;
    std::string ruta;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--tokens") {
            mostrarTokens = true;
        } else if (arg == "--tree") {
            mostrarArbol = true;
        } else if (arg == "--tablas") {
            mostrarTablas = true;
        } else if (arg == "--cuadruplos") {
            mostrarCuadruplos = true;
        } else {
            ruta = arg;
        }
    }

    if (ruta.empty()) {
        std::cerr << "Uso: " << argv[0] << " [--tokens] [--tree] [--tablas] [--cuadruplos] archivo\n";
        return 1;
    }

    std::ifstream archivo(ruta);
    if (!archivo) {
        std::cerr << "No se pudo abrir el archivo: " << ruta << "\n";
        return 1;
    }

    //Scanner: convierte el texto en tokens
    ANTLRInputStream entrada(archivo);
    BabyDuckLexer lexer(&entrada);
    ReporteErrores erroresLexicos("léxico");
    lexer.removeErrorListeners();
    lexer.addErrorListener(&erroresLexicos);

    CommonTokenStream tokens(&lexer);
    tokens.fill();

    if (mostrarTokens) {
        std::cout << "--- Tokens ---\n";
        for (Token* token : tokens.getTokens()) {
            std::string nombre = token->getType() == Token::EOF
                ? "EOF"
                : std::string(lexer.getVocabulary().getSymbolicName(token->getType()));
            std::cout << token->getLine() << ":" << token->getCharPositionInLine() + 1
                      << "\t" << nombre << "\t" << token->getText() << "\n";
        }
    }

    // Parser: revisa que los tokens sigan la gramatica y se arma el arbol
    BabyDuckParser parser(&tokens);
    ReporteErrores erroresSintacticos("sintáctico");
    parser.removeErrorListeners();
    parser.addErrorListener(&erroresSintacticos);

    tree::ParseTree* arbol = parser.programa();

    if (mostrarArbol) {
        std::cout << "--- Árbol ---\n"
                  << arbol->toStringTree(&parser, true) << "\n";
    }

    int totalErrores = erroresLexicos.errores + erroresSintacticos.errores;

    // Semantica: solo si el programa es sintacticamente correcto. Se recorre el arbol y los puntos neuralgicos llenan el directorio de funciones y generan los cuadruplos
    if (totalErrores == 0) {
        GeneradorCuadruplos semantica;
        tree::ParseTreeWalker::DEFAULT.walk(&semantica, arbol);
        totalErrores += semantica.errores;

        if (mostrarTablas) {
            std::cout << "--- Directorio de funciones ---\n";
            semantica.directorio.imprimir(std::cout);
        }
        if (mostrarCuadruplos && totalErrores == 0) {
            std::cout << "--- Cuádruplos ---\n";
            semantica.codigo.imprimir(std::cout);
        }
    }
    if (totalErrores > 0) {
        std::cerr << "Programa rechazado: " << totalErrores << " error(es).\n";
        return 1;
    }

    std::cout << "Programa aceptado.\n";
    return 0;
}
