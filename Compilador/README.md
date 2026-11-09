# BabyDuck: Scanner, Parser y Semántica

Scanner y parser generados con ANTLR4 a partir de `BabyDuck.g4`, más un análisis
semántico que llena el Directorio de Funciones y las Tablas de Variables.

## Requisitos (macOS)
```bash
brew install antlr antlr4-cpp-runtime
```

## Compilar y Probar
```bash
make # genera el código con ANTLR y compila ./compilador
make test # corre los programas de pruebas/ y las pruebas de tablas
make test_tablas # compila solo las pruebas del directorio de funciones
make clean # borra lo generado
```

## Uso
```bash
./compilador programa.txt # acepta o rechaza el programa
./compilador --tokens --tree programa.txt # muestra tokens y árbol
./compilador --tablas programa.txt # muestra el directorio de funciones
```

## Archivos
- `BabyDuck.g4`: tokens (expresiones regulares) y reglas gramaticales.
- `main.cpp`: corre el scanner, el parser y la semántica, y reporta errores léxicos, sintácticos y semánticos con línea y columna.
- `semantica.hpp`: puntos neurálgicos (listener de ANTLR) que crean y llenan el directorio y validan declaraciones.
- `cubo_semantico.hpp`: cubo semántico (tipo resultado de cada operador).
- `tablas.hpp`: Directorio de Funciones y Tablas de Variables (ver abajo).
- `pruebas/`: programas que deben aceptarse (`validos/`) y rechazarse (`invalidos/`).

## Directorio de Funciones y Tablas de Variables

`tablas.hpp` define:
- `TablaVariables`: tabla hash `nombre -> Variable{nombre, tipo, linea}` de un ámbito.
  Rechaza nombres repetidos en el mismo ámbito.
- `Funcion`: nombre, línea, tipos de los parámetros en orden y su propia `TablaVariables`
  (parámetros + variables locales). Todas las funciones son `void`.
- `DirectorioFunciones`: nombre del programa, tabla de variables globales y tabla hash
  `nombre -> Funcion`. Lleva la función actual; `buscarVariable` busca primero en la
  función actual y después en las globales (una local puede ocultar a una global).

Las llena `semantica.hpp`. Validaciones que hace al dar de alta: variable o parámetro repetido en el mismo ámbito,
función repetida, y nombres de función/global que choquen entre sí o con el nombre del
programa. Los errores se lanzan como `ErrorSemantico` con la línea.
