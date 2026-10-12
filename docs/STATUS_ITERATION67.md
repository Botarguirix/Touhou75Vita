# Iteración 67 — Heap Reuse / 01.71

## Evidencia física de la 66

Build iteration66-buffered-startup-r1 confirmado por el log; la foto adjunta era
de la 65. El principal alcanzó 9483 llamadas, 2467 D3D, seis DInput, 199 DirectSound,
964 HeapAlloc y 214 operaciones adicionales de heap. Treinta reanudaciones,
tiempo 17768489 us. log.txt llegó a LoadEffect y ...OK. La última asignación de
16 bytes devolvió cero, con 630 bloques vivos; luego hubo CPU FAULT en 00011F1F.
El runtime no reportó agotamiento del allocador JIT. La causa exacta del salto
bajo aún no se ha reconstruido, pero sí se confirmó el fallo de HeapAlloc previo.
BGM nativo: 100 bloques, cero late_fill_blocks y cierre del puerto correcto.

El allocator era monotónico en C00000..1400000 (8 MiB): HeapFree eliminaba la
metadata sin recuperar capacidad y HeapReAlloc movido tampoco recuperaba el
bloque anterior. La 67 corrige ese desperdicio antes de aumentar la arena.

## Cambios

HeapAlloc busca primero un rango libre suficiente, lo divide si sobra y conserva
alineación de 16 bytes. Si no hay uno, utiliza el extremo todavía no reservado.
HeapFree devuelve la capacidad completa, une rangos vecinos y reduce el extremo
si libera el último tramo. Los punteros inválidos/doble liberación siguen siendo
errores de contrato antes de insertar rangos. Los bloques vivos no se reutilizan.

HeapReAlloc en sitio conserva capacidad; si debe mover, reserva el nuevo rango,
copia y comprueba bytes conservados y luego devuelve el rango viejo. In-place-only
y zero-memory se mantienen. No se modifica la ubicación de staging gráfico/PCM,
ni el tamaño del heap/arena. Los bloques ambientales siguen su reserva separada
sobre el mismo extremo; no se insertan en rangos generales al liberar entorno.

startup_heap_reuse y startup_heap_reclaim registran direcciones, tamaños y extremo.
Si HeapAlloc no encuentra capacidad, la frontera capacity_exhausted detiene la
llamada con sus argumentos conservados antes de devolver un NULL que conduzca a
una ruta de fallo no soportada. Esto es una limitación explícita de la prueba,
no una implementación completa de excepciones Windows por falta de memoria.
HeapReAlloc sin espacio sigue devolviendo fallo con bloque original intacto.

## Paquete y captura

VitaSDK compiló sin advertencias; diff --check limpio. Inspección ZIP CRC,
SFO 01.71/T075VITA1 y cuatro PNG indexados de 8 bits sin entrelazado: aprobada.
No se ejecutaron nuevas pruebas unitarias; reutilización y avance pendientes de
captura física. Mantiene controles de música/visor, 32 tandas, tiempo 45 s y
watchdog 60 s. No se afirma título original ni gameplay.

Build iteration67-heap-reuse-r1.
VPK artifacts/iteration67/Touhou75Vita-iteration67.vpk.
SHA256: e2a3e7e91a1a9d25c01eded4b8211d0c9e9db6236bb2bcff412d5119ff08fd90.

Enviar iteration67.log, iteration67-runtime.log, iteration67-watchdog.log,
iteration67-bgm.log, log.txt y foto de la 67. Revisar reuse/reclaim, posible
capacity_exhausted y startup_stop_import; comprobar qué sigue a LoadEffect.
