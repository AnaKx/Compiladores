# BabyDuck

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
make test_cuadruplos # compila solo las pruebas de pilas y fila
make cuadruplos # despliega cada programa de prueba y su fila de cuádruplos
make clean # borra lo generado
```

## Uso
```bash
./compilador programa.txt # acepta o rechaza el programa
./compilador --tokens --tree programa.txt # muestra tokens y árbol
./compilador --tablas programa.txt # muestra el directorio de funciones
./compilador --cuadruplos programa.txt # muestra los cuádruplos generados
```

## Archivos
- `BabyDuck.g4`: tokens (expresiones regulares) y reglas gramaticales.
- `main.cpp`: corre el scanner, el parser y la semántica, y reporta errores léxicos, sintácticos y semánticos con línea y columna.
- `semantica.hpp`: puntos neurálgicos (listener de ANTLR) que crean y llenan el directorio y validan declaraciones.
- `generador.hpp`: puntos neurálgicos que traducen expresiones, estatutos lineales, `if`/`while` y llamadas a cuádruplos (ver abajo).
- `cubo_semantico.hpp`: cubo semántico (tipo resultado de cada operador).
- `tablas.hpp`: Directorio de Funciones y Tablas de Variables (ver abajo).
- `stack.hpp`, `queue.hpp`: plantillas `Stack` y `Queue` de la Tarea 1.
- `cuadruplos.hpp`: pilas de operadores, operandos y tipos, y fila de cuádruplos (ver abajo).
- `pruebas/`: programas que deben aceptarse (`validos/`) y rechazarse (`invalidos/`), y
  programas con sus cuádruplos esperados (`cuadruplos/*.txt` + `*.esperado`).

## Directorio de Funciones y Tablas de Variables

`tablas.hpp` define:
- `TablaVariables`: tabla hash `nombre -> Variable{nombre, tipo, linea}` de un ámbito.
  Rechaza nombres repetidos en el mismo ámbito.
- `Funcion`: nombre, línea, tipos de los parámetros en orden y su propia `TablaVariables`
  (parámetros + variables locales) y su tipo de retorno (`void`, `int` o `float`). Una función
  con tipo deja su valor en una variable global con su nombre (la escribe `RETURN`).
- `DirectorioFunciones`: nombre del programa, tabla de variables globales y tabla hash
  `nombre -> Funcion`. Lleva la función actual; `buscarVariable` busca primero en la
  función actual y después en las globales (una local puede ocultar a una global).

Las llena `semantica.hpp`. Validaciones que hace al dar de alta: variable o parámetro repetido en el mismo ámbito,
función repetida, y nombres de función/global que choquen entre sí o con el nombre del
programa. Los errores se lanzan como `ErrorSemantico` con la línea.

## Pilas y Fila de Cuádruplos

`stack.hpp` y `queue.hpp` son las plantillas `Stack<T>` y `Queue<T>` de la Tarea 1:
- `Stack<T>` (LIFO, sobre `std::vector`): `push`, `pop`, `top`, `empty`, `size`, `clear`.
- `Queue<T>` (FIFO, buffer circular que crece al doble): `enqueue`, `dequeue`, `front`,
  `empty`, `size`. Para los cuádruplos se le agregó `at(i)`: acceso por posición sin sacar
  elementos, para rellenar saltos pendientes e imprimir la fila.

`cuadruplos.hpp` las usa para definir:
- `Cuadruplo`: `(op, izq, der, res)`.
- `sacar(pila)`: hace `top()` + `pop()` y regresa el elemento.
- `EstructurasCodigo`: agrupa `poper` (`Stack<Op>`), `pilaO` (`Stack<std::string>`),
  `ptipos` (`Stack<Tipo>`), `psaltos` (`Stack<size_t>`, saltos pendientes) y
  `cuadruplos` (`Queue<Cuadruplo>`). `pushOperando`/`popOperando`
  mueven operando y tipo juntos; `generar` agrega un cuádruplo, `siguiente` da el
  índice del próximo y `rellenar` pone el destino de un salto.

`Op::FONDO_FALSO` representa el `(` en la pila de operadores; no está en el cubo semántico.

## Traducción a Cuádruplos

`generador.hpp` (`GeneradorCuadruplos`, hereda de `AnalizadorSemantico`) agrega los puntos
neurálgicos que usan las pilas `POper`, `PilaO`, `PTipos` y la fila de cuádruplos:

| PN | Dónde | Acción |
|---|---|---|
| E1–E3 | al leer `+ -`, `* /`, `> <` | push del operador a POper |
| E4 | `(` | push del fondo falso |
| E5 | fin de factor | `)` saca el fondo falso; `ID`/constante entran a PilaO con su tipo (`-x` genera `NEG`); luego, si el tope es `*` o `/`, genera |
| E6 | fin de término | si el tope es `+` o `-`, genera |
| E7 | fin de expresión | si el tope es `>` o `<`, genera |
| L1 | `ID = expresion ;` | `(=, valor, _, ID)` |
| L2 | `read(ID);` | `(read, _, _, ID)` |
| L3 | cada elemento de `print` | `(print, _, _, valor)` o `(print, _, _, "letrero")` |
| C1 | tras la condición de `if`/`while` | revisa que sea `bool`; `(GOTOF, cond, _, ?)` y push a PSaltos |
| C2 | `else` | `(GOTO, _, _, ?)`; rellena el GOTOF al inicio del else |
| C3 | fin del `if` | rellena el salto pendiente con el siguiente cuádruplo |
| C4 | inicio del `while` | push a PSaltos del punto de regreso |
| C5 | fin del `while` | `(GOTO, _, _, regreso)`; rellena el GOTOF |
| F1 | `ID (` | `(ERA, _, _, función)`; fondo falso para los argumentos |
| F2 | cada argumento | revisa su tipo contra el parámetro; `(PARAM, valor, _, parK)` |
| F3 | `)` de la llamada | `(GOSUB, función, _, inicio)` |
| F4 | `(void \| type) ID` | guarda en el directorio el cuádruplo donde empieza la función |
| F5 | fin de la función | `(ENDFUNC, _, _, _)` |
| P1 | `program ID` | `(GOTO, _, _, ?)` hacia main |
| P2 | inicio de `main` | rellena el GOTO inicial |
| P3 | `end` | `(END, _, _, _)` |
| R1 | `return expresion ;` | revisa el tipo contra el de la función; `(RETURN, valor, _, función)` |

En `factor`, una llamada a una función con tipo genera, después del `GOSUB`,
`(=, función, _, _tN)` y mete `_tN` a PilaO con el tipo de retorno.

"Genera" = saca operador y dos operandos, revisa el cubo semántico, emite
`(op, izq, der, _tN)` y mete el temporal `_tN` a PilaO. Así `* /` tienen más precedencia que
`+ -`, y éstos más que `> <`, y todos son asociativos a la izquierda. Los temporales empiezan
con `_` para no chocar con IDs del programa.

Errores de tipo que se detectan: operación sin tipo en el cubo (p. ej. `bool + int`),
asignación incompatible (`int = float`, `int = bool`), condición de `if`/`while` que no es
`bool`, argumento de tipo incompatible con su parámetro, usar una función `void` como valor
dentro de una expresión, `return` fuera de una función o en una función `void` (PN9),
`return` con un tipo incompatible con el de la función (R1) y función con tipo sin `return`
(PN5).
