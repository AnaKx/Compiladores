# ============================================================================
# Makefile
# Construcción del proyecto (no es una fase)
#
# Qué hace:
#   Da las instrucciones para `make`: correr ANTLR sobre la gramática, compilar
#   ./compilador y los dos programas de prueba, y correr las pruebas.
#
# Papel en el proceso:
#   Es lo primero que se usa: convierte todos los archivos en programas.
#   Comandos: make | make test | make cuadruplos | make clean
#   Requiere: brew install antlr antlr4-cpp-runtime
# ============================================================================

LEXSIN := 1_lexico_sintaxis
SEM := 2_semantica
COD := 3_codigo_intermedio
EST := estructuras

# Con -I de cada carpeta, los #include "archivo.hpp" funcionan sin escribir la carpeta
INCLUDES := -I$(SEM) -I$(COD) -I$(EST)

BREW := $(shell brew --prefix)
CXX := clang++
CXXFLAGS := -std=c++17 -Wall -I$(BREW)/include/antlr4-runtime -Igenerated $(INCLUDES)
LDFLAGS := -L$(BREW)/lib -lantlr4-runtime -Wl,-rpath,$(BREW)/lib

GENERADOS := generated/BabyDuckLexer.cpp generated/BabyDuckParser.cpp \
             generated/BabyDuckListener.cpp generated/BabyDuckBaseListener.cpp
FUENTES := main.cpp $(GENERADOS)

ESTRUCTURAS := $(EST)/stack.hpp $(EST)/queue.hpp $(EST)/hash.hpp
SEMANTICO := $(SEM)/cubo_semantico.hpp $(SEM)/tablas.hpp $(SEM)/semantica.hpp
CODIGO := $(COD)/memoria.hpp $(COD)/cuadruplos.hpp $(COD)/generador.hpp

compilador: $(FUENTES) $(SEMANTICO) $(CODIGO) $(ESTRUCTURAS)
	$(CXX) $(CXXFLAGS) $(FUENTES) -o $@ $(LDFLAGS)

# ANTLR lee la gramatica y genera el codigo C++ del scanner, del parser y del listener
$(GENERADOS): $(LEXSIN)/BabyDuck.g4
	antlr -Dlanguage=Cpp -listener -o generated -Xexact-output-dir $(LEXSIN)/BabyDuck.g4

# Las pruebas unitarias se compilan junto a su código, en la carpeta de su fase
TEST_TABLAS := pruebas/$(SEM)/test_tablas
TEST_CUADRUPLOS := pruebas/$(COD)/test_cuadruplos

test: compilador $(TEST_TABLAS) $(TEST_CUADRUPLOS)
	./pruebas/correr_pruebas.sh
	./$(TEST_TABLAS)
	./$(TEST_CUADRUPLOS)

# Despliegue la fila de cuadruplos de los programas de prueba
cuadruplos: compilador
	./pruebas/mostrar_cuadruplos.sh

# Pruebas del directorio de funciones y tablas de variables (sin ANTLR)
test_tablas: $(TEST_TABLAS)
$(TEST_TABLAS): $(TEST_TABLAS).cpp $(SEM)/tablas.hpp $(SEM)/cubo_semantico.hpp $(COD)/memoria.hpp $(EST)/hash.hpp
	$(CXX) -std=c++17 -Wall $(INCLUDES) $@.cpp -o $@

# Pruebas de las pilas y la fila de cuadruplos (sin ANTLR)
test_cuadruplos: $(TEST_CUADRUPLOS)
$(TEST_CUADRUPLOS): $(TEST_CUADRUPLOS).cpp $(COD)/cuadruplos.hpp $(COD)/memoria.hpp $(SEM)/cubo_semantico.hpp $(ESTRUCTURAS)
	$(CXX) -std=c++17 -Wall $(INCLUDES) $@.cpp -o $@

clean:
	rm -rf generated compilador $(TEST_TABLAS) $(TEST_CUADRUPLOS)

.PHONY: test clean cuadruplos test_tablas test_cuadruplos
