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
./compilador --tablas --cuadruplos programa.txt # direcciones de cada variable/constante + cuádruplos
```

## Archivos

Organizados en una carpeta por fase del compilador:

```
Compilador/
├── main.cpp                  corre todas las fases en orden
├── Makefile
├── 1_lexico_sintaxis/        Fase 1-2: scanner y parser
│   └── BabyDuck.g4
├── 2_semantica/              Fase 3: análisis semántico
│   ├── cubo_semantico.hpp
│   ├── tablas.hpp
│   └── semantica.hpp
├── 3_codigo_intermedio/      Fase 4: generación de cuádruplos
│   ├── memoria.hpp
│   ├── cuadruplos.hpp
│   └── generador.hpp
├── estructuras/              Stack, Queue y Hash de la Tarea 1 (las usan varias fases)
│   ├── stack.hpp
│   ├── queue.hpp
│   └── hash.hpp
├── generated/                código que genera ANTLR (no se edita)
└── pruebas/
```

- `1_lexico_sintaxis/BabyDuck.g4`: tokens (expresiones regulares) y reglas gramaticales.
- `main.cpp`: corre el scanner, el parser y la semántica, y reporta errores léxicos, sintácticos y semánticos con línea y columna.
- `2_semantica/semantica.hpp`: puntos neurálgicos (listener de ANTLR) que crean y llenan el directorio y validan declaraciones.
- `2_semantica/cubo_semantico.hpp`: cubo semántico (tipo resultado de cada operador).
- `2_semantica/tablas.hpp`: Directorio de Funciones y Tablas de Variables (ver abajo).
- `3_codigo_intermedio/generador.hpp`: puntos neurálgicos que traducen expresiones, estatutos lineales, `if`/`while` y llamadas a cuádruplos (ver abajo).
- `3_codigo_intermedio/memoria.hpp`: memoria virtual: segmentos, contadores de direcciones y tabla de constantes (ver abajo).
- `3_codigo_intermedio/cuadruplos.hpp`: pilas de operadores, operandos y tipos, y fila de cuádruplos (ver abajo).
- `estructuras/stack.hpp`, `queue.hpp`, `hash.hpp`: plantillas `Stack`, `Queue` y `Hash` de la Tarea 1.

Los `#include` solo llevan el nombre del archivo (`#include "tablas.hpp"`): el Makefile pasa
`-I` con cada carpeta para que el compilador los encuentre.

### Pruebas

`pruebas/` sigue las mismas carpetas por fase:

```
pruebas/
├── correr_pruebas.sh             corre todos los programas de prueba (lo usa make test)
├── mostrar_cuadruplos.sh         imprime programa + directorio + cuádruplos (make cuadruplos)
├── salida_cuadruplos.txt         salida guardada de make cuadruplos
├── validos/                      programas completos que deben aceptarse
├── 1_lexico_sintaxis/
│   ├── errores_lexicos/          deben rechazarse con un "Error léxico"
│   └── errores_sintacticos/      deben rechazarse con un "Error sintáctico"
├── 2_semantica/
│   ├── errores/                  deben rechazarse con un "Error semántico"
│   └── test_tablas.cpp           prueba unitaria de hash.hpp y tablas.hpp
└── 3_codigo_intermedio/
    ├── programas/                *.txt + *.esperado (salida de --tablas --cuadruplos)
    └── test_cuadruplos.cpp       prueba unitaria de pilas, fila y memoria virtual
```

El nombre de cada programa con error dice qué error tiene (p. ej.
`2_semantica/errores/var_no_declarada.txt`). `correr_pruebas.sh` revisa que se rechace
**y** que lo detecte la fase correcta. Las pruebas unitarias se compilan junto a su `.cpp`.

## Directorio de Funciones y Tablas de Variables

`tablas.hpp` define:
- Todas las tablas hash (variables, funciones y constantes) son el `Hash<K, V>` de la Tarea 1
  (`hash.hpp`). Para el compilador se le agregó `find` const y `forEach` (recorrer todos los
  pares). `put` puede mover los valores al crecer, así que un puntero de `find` solo se usa
  hasta la siguiente alta en esa misma tabla.
- `TablaVariables`: tabla hash `nombre -> Variable{nombre, tipo, linea, direccion}` de un ámbito.
  Rechaza nombres repetidos en el mismo ámbito.
- `Funcion`: nombre, línea, cuádruplo donde empieza, tipos y direcciones de los parámetros en
  orden, su propia `TablaVariables` (parámetros + variables locales), su tipo de retorno
  (`void`, `int` o `float`) y sus recursos (cuántos locales y temporales de cada tipo usa; es lo
  que reservará `ERA`). Una función con tipo deja su valor en una variable global con su
  nombre (la escribe `RETURN`), con dirección `direccionRetorno` en el segmento global.
- `DirectorioFunciones`: nombre del programa, tabla de variables globales y tabla hash
  `nombre -> Funcion`. Lleva la función actual; `buscarVariable` busca primero en la
  función actual y después en las globales (una local puede ocultar a una global).

El directorio también guarda la `MemoriaVirtual`: al dar de alta una global, parámetro o local
le pide su dirección, y el generador le pide temporales y constantes. `--tablas` imprime la
dirección de cada variable, los recursos de cada función y de main, y la tabla de constantes.

Las llena `semantica.hpp`. Validaciones que hace al dar de alta: variable o parámetro repetido en el mismo ámbito,
función repetida, y nombres de función/global que choquen entre sí o con el nombre del
programa. Los errores se lanzan como `ErrorSemantico` con la línea.

## Memoria Virtual

`memoria.hpp` reparte direcciones por segmento y tipo (1000 por cada uno):

| Segmento | int | float | bool | string |
|---|---|---|---|---|
| Global | 1000–1999 | 2000–2999 | | |
| Local | 3000–3999 | 4000–4999 | | |
| Temporal | 5000–5999 | 6000–6999 | 7000–7999 | |
| Constante | 8000–8999 | 9000–9999 | | 10000–10999 |

- **Global**: variables del programa y el valor de regreso de cada función con tipo.
- **Local**: parámetros y variables de una función. Se reinicia al empezar cada función: cada
  llamada tendrá su propio registro de activación, así que `a` de una función y `n` de otra
  pueden ser ambas `3000`.
- **Temporal**: resultados intermedios (`bool` sólo existe aquí, lo producen `>` y `<`). Se
  reinicia por función y para main, porque en una función recursiva cada llamada necesita sus
  propios temporales.
- **Constante**: tabla hash (`Hash`) `valor -> dirección`; un valor repetido usa la misma dirección. Una
  constante con signo (`-2.5`) es su propia constante. Los letreros de `print` también son
  constantes.

Con la dirección se puede saber el segmento y el tipo (`memoria::segmentoDe`,
`memoria::tipoDe`), lo que usará la máquina virtual. Si un segmento se llena se reporta un
error semántico.

## Pilas y Fila de Cuádruplos

`stack.hpp` y `queue.hpp` son las plantillas `Stack<T>` y `Queue<T>` de la Tarea 1:
- `Stack<T>` (LIFO, sobre `std::vector`): `push`, `pop`, `top`, `empty`, `size`, `clear`.
- `Queue<T>` (FIFO, buffer circular que crece al doble): `enqueue`, `dequeue`, `front`,
  `empty`, `size`. Para los cuádruplos se le agregó `at(i)`: acceso por posición sin sacar
  elementos, para rellenar saltos pendientes e imprimir la fila.

`cuadruplos.hpp` las usa para definir:
- `Cuadruplo`: `(op, izq, der, res)`.
- `sacar(pila)`: hace `top()` + `pop()` y regresa el elemento.
- `EstructurasCodigo`: agrupa `poper` (`Stack<Op>`), `pilaO` (`Stack<int>`, direcciones),
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
| E5 | fin de factor | `)` saca el fondo falso; `ID` entra a PilaO con su dirección y tipo, una constante con su dirección de la tabla de constantes (`-x` genera `NEG`); luego, si el tope es `*` o `/`, genera |
| E6 | fin de término | si el tope es `+` o `-`, genera |
| E7 | fin de expresión | si el tope es `>` o `<`, genera |
| L1 | `ID = expresion ;` | `(=, valor, _, ID)` |
| L2 | `read(ID);` | `(read, _, _, ID)` |
| L3 | cada elemento de `print` | `(print, _, _, valor)`; un letrero usa su dirección de constante string |
| C1 | tras la condición de `if`/`while` | revisa que sea `bool`; `(GOTOF, cond, _, ?)` y push a PSaltos |
| C2 | `else` | `(GOTO, _, _, ?)`; rellena el GOTOF al inicio del else |
| C3 | fin del `if` | rellena el salto pendiente con el siguiente cuádruplo |
| C4 | inicio del `while` | push a PSaltos del punto de regreso |
| C5 | fin del `while` | `(GOTO, _, _, regreso)`; rellena el GOTOF |
| F1 | `ID (` | `(ERA, _, _, función)`; fondo falso para los argumentos |
| F2 | cada argumento | revisa su tipo contra el parámetro; `(PARAM, valor, _, dirParámetro)` (dirección local del k-ésimo parámetro) |
| F3 | `)` de la llamada | `(GOSUB, función, _, inicio)` |
| F4 | `(void \| type) ID` | guarda en el directorio el cuádruplo donde empieza la función |
| F5 | fin de la función | `(ENDFUNC, _, _, _)` |
| P1 | `program ID` | `(GOTO, _, _, ?)` hacia main |
| P2 | inicio de `main` | rellena el GOTO inicial |
| P3 | `end` | `(END, _, _, _)` |
| R1 | `return expresion ;` | revisa el tipo contra el de la función; `(RETURN, valor, _, dirRegreso)` |

En `factor`, una llamada a una función con tipo genera, después del `GOSUB`,
`(=, dirRegreso, _, tN)` y mete el temporal `tN` a PilaO con el tipo de retorno.

"Genera" = saca operador y dos operandos, revisa el cubo semántico, pide un temporal del tipo
resultante al ámbito actual, emite `(op, izq, der, tN)` y mete `tN` a PilaO. Así `* /` tienen
más precedencia que `+ -`, y éstos más que `> <`, y todos son asociativos a la izquierda.

Todos los operandos de los cuádruplos son direcciones virtuales; los destinos de
`GOTO`/`GOTOF`/`GOSUB` son números de cuádruplo y `ERA`/`GOSUB` nombran la función (la
máquina virtual busca en el directorio su inicio y sus recursos). Ejemplo
(`pruebas/3_codigo_intermedio/programas/retorno.txt`, `k` local en 3000, regreso de `factorial` en 1002):

```
return k * factorial(k - 1);
   4: (ERA, _, _, factorial)
   5: (-, 3000, 8001, 5000)     k - 1
   6: (PARAM, 5000, _, 3000)    al parámetro k de la nueva llamada
   7: (GOSUB, factorial, _, 1)
   8: (=, 1002, _, 5001)        copia el valor regresado a un temporal
   9: (*, 3000, 5001, 5002)
  10: (RETURN, 5002, _, 1002)
```

Errores de tipo que se detectan: operación sin tipo en el cubo (p. ej. `bool + int`),
asignación incompatible (`int = float`, `int = bool`), condición de `if`/`while` que no es
`bool`, argumento de tipo incompatible con su parámetro, usar una función `void` como valor
dentro de una expresión, `return` fuera de una función o en una función `void` (PN9),
`return` con un tipo incompatible con el de la función (R1) y función con tipo sin `return`
(PN5).
