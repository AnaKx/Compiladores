# Compila el scanner y el parser generados por ANTLR4.
# Requiere: brew install antlr antlr4-cpp-runtime

BREW := $(shell brew --prefix)
CXX := clang++
CXXFLAGS := -std=c++17 -Wall -I$(BREW)/include/antlr4-runtime -Igenerated
LDFLAGS := -L$(BREW)/lib -lantlr4-runtime -Wl,-rpath,$(BREW)/lib

GENERADOS := generated/BabyDuckLexer.cpp generated/BabyDuckParser.cpp
FUENTES := main.cpp $(GENERADOS)

compilador: $(FUENTES)
	$(CXX) $(CXXFLAGS) $(FUENTES) -o $@ $(LDFLAGS)

# ANTLR lee la gramatica y genera el codigo C++ del scanner y del parser
$(GENERADOS): BabyDuck.g4
	antlr -Dlanguage=Cpp -no-listener -o generated -Xexact-output-dir BabyDuck.g4

test: compilador
	./pruebas/correr_pruebas.sh

clean:
	rm -rf generated compilador

.PHONY: test clean
