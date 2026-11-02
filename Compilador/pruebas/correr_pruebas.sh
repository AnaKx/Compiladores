#!/bin/bash
# Los programas en validos/ deben aceptarse y los de invalidos/ deben rechazarse.
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

echo "Fallas: $fallas"
[ "$fallas" -eq 0 ]
