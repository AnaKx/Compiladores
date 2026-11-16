#!/bin/bash
# Despliega el programa fuente y la fila final de cuádruplos de cada programa de prueba.
# Uso: ./pruebas/mostrar_cuadruplos.sh [archivo ...]   (sin argumentos: validos/ y cuadruplos/)
cd "$(dirname "$0")/.."

if [ $# -gt 0 ]; then
    archivos=("$@")
else
    archivos=(pruebas/validos/*.txt pruebas/cuadruplos/*.txt)
fi

for f in "${archivos[@]}"; do
    echo "=================================================================="
    echo " $f"
    echo "=================================================================="
    nl -ba "$f"
    echo
    ./compilador --cuadruplos "$f" 2>&1
    echo
done
