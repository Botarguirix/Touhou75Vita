# Iteración 76 — EXE Transition / 01.80

## Resultado físico de la 75

Se presentaron ocho frames originales. Cada llamada nativa set/vblank/query
devolvió RC 0 y matches:yes, alternando los dos buffers de scanout.
El worker original 12 (TIB 760000) ejecutó ocho SetEvent sobre AB2004;
las generaciones 1..8 coinciden con ocho waits auto-reset consumidos.
Cada retorno fue WAIT_OBJECT_0 con limpieza de 12 bytes, conservando
GPR/EIP/flags/TIB/FPU, call frame y LastError del main.

| Frame | Scanout FNV-1a |
|---|---|
| 1 | B047B6F7 |
| 2 | 44B6FA68 |
| 3 | 0BA47480 |
| 4 | AF928E16 |
| 5 | 686641A5 |
| 6 | 5BC8CC09 |
| 7 | E35328A7 |
| 8 | 1CF81F90 |

La parada fue startup_d3d8_present_scope_limit=frames:8 maximum:8,
antes del noveno Present. EIP BFC0F0, ESP 9FEC18 y EBP 9FEC34.
30 slices, 37373480 us, sin fault ni límite de CPU; watchdog desarmado.
2852 servicios D3D, seis DInput y 199 DirectSound. log.txt conserva las
inicializaciones y LoadEffect OK. La captura corresponde al fade del logo,
no al menú jugable.

## Cambio para avanzar

Los flags de la build 75 eran -Wl,-q -std=gnu++17 -Wall -Wextra;
CMAKE_BUILD_TYPE estaba vacío. El código propio Vita, incluidos raster y
conversión de scanout, se compilaba sin optimización. El runtime WinVita ya
usaba -O2: el hallazgo afecta a la aplicación, no a una nueva traducción del EXE.

La 76 establece RelWithDebInfo por defecto cuando no hay configuración
explícita. El VPK local se compiló con -O2 -g -DNDEBUG y
-fno-fast-math -ffp-contract=off. Mantiene el algoritmo de píxeles y las
guardas ABI/memoria. La reducción de tiempo es una expectativa de la
optimización; falta medir su magnitud en la Vita.

- Permite 16 Present y 16 continuaciones del wait observado, con límites
  compartidos en startup_limits.h y contador visual /16.
- Conserva 45 s de cap, watchdog de 60 s, máximo de 64 slices, 64 draws
  y 16777216 píxeles cubiertos. La siguiente frontera no soportada detiene
  la ejecución y mantiene sus datos para diagnosticar.
- Registra tiempo del raster con hashes, preparación/display/captura de
  Present y ciclo timer+main. Son mediciones de etapas diferentes.
- Lee sin modificar el contador original en 66C240 y la palabra de estado
  [EBP-3C] del main en la espera conocida.
- Conserva diagnóstico EXE y X para salir. LiveArea identifica 76 / 01.80.

El análisis estático muestra que 431F60 decrementa 66C240 después de Present
y termina la transición al pasar a -1. La rutina 432410 calcula el brillo
como 255 - contador*24. El noveno dibujo pendiente de la 75 usa FFCFCFCF
(brillo 207), compatible con contador 2. Esto sugiere que ampliar la corrida
puede superar el fade; no demuestra cuál será la próxima escena ni que la
76 llegue al menú. Los nuevos logs observarán ese cambio real del EXE.

## Comprobaciones y entrega

18 grupos portables propios aprobaron con ASan/UBSan. Los seis del renderer
aprobaron a -O0 y -O2 con salida y hashes de patrones idénticos. Véase
[lista y límites](EXE_CHECKLIST_ITERATION76.md).
La compilación VitaSDK fue exitosa, sin advertencias en esta recompilación.
ZIP CRC correcto, SFO 01.80/T075VITA1, cuatro PNG indexados de ocho bits y
sin entrelazado. La aplicación optimizada y su transición extendida están
pendientes de validación física; las comprobaciones PC no ejecutan el EXE.

Build: iteration76-exe-startup-transition-r1.
VPK local: artifacts/iteration76/Touhou75Vita-iteration76.vpk.
SHA256: d1c802735e16749a3988434671067d295b65e0876cc50b04e2f2eb9e72bc5c2c.

## Próxima corrida en Vita

1. Instalar 76/01.80 y conservar los archivos originales del juego.
2. Esperar al diagnóstico; fotografiar STOP, contador /16 y captura EXE. X sale.
3. Enviar iteration76.log, iteration76-runtime.log, iteration76-watchdog.log,
   log.txt y captura.
4. Revisar optimización habilitada, hashes iniciales, señales/contexto,
   contador de transición, tiempos y próxima frontera.
5. 16/16 y STOP Present indican el cap previsto antes del frame 17. Si aparece
   otra API/estado, sus argumentos definirán el contrato de la siguiente iteración.

## Referencias y alcance

La configuración por defecto sigue
[CMAKE_BUILD_TYPE](https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html).
La elección de optimización y los flags FP siguen las opciones oficiales de
[GCC](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html).
La sincronización original confirmada en la 75 está descrita en
[STATUS_ITERATION75](STATUS_ITERATION75.md).
Los proyectos externos y descompiladores siguen en
[RESEARCH_EXE_ITERATION73](RESEARCH_EXE_ITERATION73.md).
No se incorporó lógica de juego de otro título. Scheduler continuo, entrada,
mezcla de audio del EXE y combate siguen pendientes.
