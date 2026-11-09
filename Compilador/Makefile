# Compila el scanner, el parser (generados por ANTLR4) y el análisis semántico.
# Requiere: brew install antlr antlr4-cpp-runtime

BREW := $(shell brew --prefix)
CXX := clang++
CXXFLAGS := -std=c++17 -Wall -I$(BREW)/include/antlr4-runtime -Igenerated
LDFLAGS := -L$(BREW)/lib -lantlr4-runtime -Wl,-rpath,$(BREW)/lib

GENERADOS := generated/BabyDuckLexer.cpp generated/BabyDuckParser.cpp \
             generated/BabyDuckListener.cpp generated/BabyDuckBaseListener.cpp
FUENTES := main.cpp $(GENERADOS)

compilador: $(FUENTES) semantica.hpp tablas.hpp cubo_semantico.hpp
	$(CXX) $(CXXFLAGS) $(FUENTES) -o $@ $(LDFLAGS)

# ANTLR lee la gramatica y genera el codigo C++ del scanner, del parser y del listener
$(GENERADOS): BabyDuck.g4
	antlr -Dlanguage=Cpp -listener -o generated -Xexact-output-dir BabyDuck.g4

test: compilador test_tablas
	./pruebas/correr_pruebas.sh
	./test_tablas

# Pruebas del directorio de funciones y tablas de variables (no necesita ANTLR)
test_tablas: pruebas/test_tablas.cpp tablas.hpp cubo_semantico.hpp
	$(CXX) -std=c++17 -Wall -I. pruebas/test_tablas.cpp -o $@

clean:
	rm -rf generated compilador test_tablas

.PHONY: test clean
