#!/bin/bash
# Los programas en validos/ deben aceptarse y los de invalidos/ deben rechazarse.
# En cuadruplos/, la salida de --cuadruplos debe ser igual al archivo .esperado.
cd "$(dirname "$0")/.."
fallas=0

for f in pruebas/validos/*.txt; do
    if ./compilador "$f" > /dev/null 2>&1; then
        echo "OK     $f"
    else
        echo "FALLA  $f (debía aceptarse)"; fallas=$((fallas + 1))
    fi
done

for f in pruebas/invalidos/*.txt; do
    if ./compilador "$f" > /dev/null 2>&1; then
        echo "FALLA  $f (debía rechazarse)"; fallas=$((fallas + 1))
    else
        echo "OK     $f"
    fi
done

for f in pruebas/cuadruplos/*.txt; do
    if ./compilador --cuadruplos "$f" 2>&1 | diff -u "${f%.txt}.esperado" - > /dev/null; then
        echo "OK     $f"
    else
        echo "FALLA  $f (cuádruplos distintos a ${f%.txt}.esperado)"; fallas=$((fallas + 1))
    fi
done

echo "Fallas: $fallas"
[ "$fallas" -eq 0 ]
