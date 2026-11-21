#!/bin/bash
# ============================================================================
# mostrar_cuadruplos.sh
# Herramienta para ver el resultado (no es parte del compilador)
#
# Qué hace:
#   Por cada programa de prueba imprime el código fuente con números de línea, el
#   directorio de funciones (con direcciones virtuales) y la fila final de cuádruplos.
#
# Papel en el proceso:
#   Sirve para revisar a ojo lo que produce el compilador. Lo corre
#   `make cuadruplos`; su salida se guardó en pruebas/salida_cuadruplos.txt.
# ============================================================================

cd "$(dirname "$0")/.."

if [ $# -gt 0 ]; then
    archivos=("$@")
else
    archivos=(pruebas/validos/*.txt pruebas/3_codigo_intermedio/programas/*.txt)
fi

for f in "${archivos[@]}"; do
    echo "=================================================================="
    echo " $f"
    echo "=================================================================="
    nl -ba "$f"
    echo
    ./compilador --tablas --cuadruplos "$f" 2>&1
    echo
done
