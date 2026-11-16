# Compila el scanner, el parser (generados por ANTLR4) y el análisis semántico.
# Requiere: brew install antlr antlr4-cpp-runtime

BREW := $(shell brew --prefix)
CXX := clang++
CXXFLAGS := -std=c++17 -Wall -I$(BREW)/include/antlr4-runtime -Igenerated
LDFLAGS := -L$(BREW)/lib -lantlr4-runtime -Wl,-rpath,$(BREW)/lib

GENERADOS := generated/BabyDuckLexer.cpp generated/BabyDuckParser.cpp \
             generated/BabyDuckListener.cpp generated/BabyDuckBaseListener.cpp
FUENTES := main.cpp $(GENERADOS)

compilador: $(FUENTES) semantica.hpp generador.hpp tablas.hpp cubo_semantico.hpp cuadruplos.hpp stack.hpp queue.hpp
	$(CXX) $(CXXFLAGS) $(FUENTES) -o $@ $(LDFLAGS)

# ANTLR lee la gramatica y genera el codigo C++ del scanner, del parser y del listener
$(GENERADOS): BabyDuck.g4
	antlr -Dlanguage=Cpp -listener -o generated -Xexact-output-dir BabyDuck.g4

test: compilador test_tablas test_cuadruplos
	./pruebas/correr_pruebas.sh
	./test_tablas
	./test_cuadruplos

# Despliegue la fila de cuadruplos de los programas de prueba
cuadruplos: compilador
	./pruebas/mostrar_cuadruplos.sh

# Pruebas del directorio de funciones y tablas de variables (sin ANTLR)
test_tablas: pruebas/test_tablas.cpp tablas.hpp cubo_semantico.hpp
	$(CXX) -std=c++17 -Wall -I. pruebas/test_tablas.cpp -o $@

# Pruebas de las pilas y la fila de cuadruplos (sin ANTLR)
test_cuadruplos: pruebas/test_cuadruplos.cpp cuadruplos.hpp cubo_semantico.hpp stack.hpp queue.hpp
	$(CXX) -std=c++17 -Wall -I. pruebas/test_cuadruplos.cpp -o $@

clean:
	rm -rf generated compilador test_tablas test_cuadruplos

.PHONY: test clean cuadruplos
