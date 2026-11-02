# BabyDuck: Scanner y Parser

Scanner y parser generados con ANTLR4 a partir de `BabyDuck.g4`.

## Requisitos (macOS)

```bash
brew install antlr antlr4-cpp-runtime
```

## Compilar y Probar

```bash
make # genera el código con ANTLR y compila ./compilador
make test # corre los programas de pruebas/validos y pruebas/invalidos
make clean # borra lo generado
```

## Uso

```bash
./compilador programa.txt # acepta o rechaza el programa
./compilador --tokens --tree programa.txt # muestra tokens y árbol
```

## Archivos

- `BabyDuck.g4`: tokens (expresiones regulares) y reglas gramaticales.
- `main.cpp`: corre el scanner y el parser, y reporta errores léxicos y sintácticos con línea y columna.
- `pruebas/`: programas que deben aceptarse (`validos/`) y rechazarse (`invalidos/`).
