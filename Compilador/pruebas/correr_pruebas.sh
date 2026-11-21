#!/bin/bash
# ============================================================================
# correr_pruebas.sh
# Pruebas de todas las fases (no es parte del compilador)
#
# Qué hace:
#   Corre ./compilador sobre cada programa de prueba y dice OK o FALLA:
#   - validos/*.txt deben aceptarse;
#   - 1_lexico_sintaxis/errores_lexicos/*.txt deben rechazarse con un "Error léxico";
#   - 1_lexico_sintaxis/errores_sintacticos/*.txt con un "Error sintáctico";
#   - 2_semantica/errores/*.txt con un "Error semántico";
#   - 3_codigo_intermedio/programas/*.txt: la salida de --tablas --cuadruplos debe
#     ser igual a su .esperado.
#
# Papel en el proceso:
#   Prueba el compilador completo, de punta a punta (todas las fases juntas), y
#   revisa que cada error lo detecte la fase correcta.
#   Lo corre `make test`. Termina con éxito solo si no hubo fallas.
# ============================================================================

cd "$(dirname "$0")/.."
fallas=0

for f in pruebas/validos/*.txt; do
    if ./compilador "$f" > /dev/null 2>&1; then
        echo "OK     $f"
    else
        echo "FALLA  $f (debía aceptarse)"; fallas=$((fallas + 1))
    fi
done

# rechazar <carpeta> <tipo de error esperado>
rechazar() {
    for f in "$1"/*.txt; do
        if salida=$(./compilador "$f" 2>&1); then
            echo "FALLA  $f (debía rechazarse)"; fallas=$((fallas + 1))
        elif ! grep -q "Error $2" <<< "$salida"; then
            echo "FALLA  $f (se rechazó, pero sin un error $2)"; fallas=$((fallas + 1))
        else
            echo "OK     $f"
        fi
    done
}

rechazar pruebas/1_lexico_sintaxis/errores_lexicos léxico
rechazar pruebas/1_lexico_sintaxis/errores_sintacticos sintáctico
rechazar pruebas/2_semantica/errores semántico

for f in pruebas/3_codigo_intermedio/programas/*.txt; do
    if ./compilador --tablas --cuadruplos "$f" 2>&1 | diff -u "${f%.txt}.esperado" - > /dev/null; then
        echo "OK     $f"
    else
        echo "FALLA  $f (cuádruplos distintos a ${f%.txt}.esperado)"; fallas=$((fallas + 1))
    fi
done

echo "Fallas: $fallas"
[ "$fallas" -eq 0 ]
