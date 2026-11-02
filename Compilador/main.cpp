// Programa principal: corre el scanner y el parser sobre un archivo fuente.
//
// Uso:
//   ./compilador archivo.txt            revisa el programa
//   ./compilador --tokens archivo.txt   además imprime la lista de tokens
//   ./compilador --tree archivo.txt     además imprime el árbol sintáctico
//
// Regresa 0 si el programa es válido y 1 si hubo errores.

#include <fstream>
#include <iostream>
#include <string>

#include "antlr4-runtime.h"
#include "BabyDuckLexer.h"
#include "BabyDuckParser.h"

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
    std::string ruta;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--tokens") {
            mostrarTokens = true;
        } else if (arg == "--tree") {
            mostrarArbol = true;
        } else {
            ruta = arg;
        }
    }

    if (ruta.empty()) {
        std::cerr << "Uso: " << argv[0] << " [--tokens] [--tree] archivo\n";
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
    if (totalErrores > 0) {
        std::cerr << "Programa rechazado: " << totalErrores << " error(es).\n";
        return 1;
    }

    std::cout << "Programa aceptado.\n";
    return 0;
}
