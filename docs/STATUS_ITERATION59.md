# Iteración 59 — Fast DAT / 01.63

## Evidencia de la 58

El EXE original atendió cuatro llamadas DirectSound y llegó a
IDirectInput8A::EnumDevices en 6.163015 segundos. Consulta observada:
this AD0000, tipo 4 GAMECTRL, callback 404010, contexto cero,
flags 1 ATTACHEDONLY. El watchdog quedó desarmado.

Los DAT BG05a y selectchar exportados son idénticos byte por byte a sus
entradas del archivo original. Los BMP BG05a/0, selectchar/0 y selectchar/5
coinciden píxel por píxel con el decodificador de referencia, incluyendo
alfa. Dimensiones: 1400x900, 512x512 y 512x1024 respectivamente.
Comparación local: artifacts/iteration59/export-comparison.json.

La sesión de música produjo 1656 bloques en 83.811593 segundos;
preparación máxima 224554 us, salida máxima 51232 us y 95 preparaciones
tardías. Esto señala competencia de trabajo/I/O durante el visor; no es
una medición directa de underruns. La música sola de la 57 fue confirmada
fluida por el usuario.

## Cambios

El visor conserva la compresión RLE original. Prepara una vista de
260x180 con escala, transparencia y tablero una sola vez al cambiar
fotograma. Cada refresco copia las filas preparadas, eliminando el
reescalado y la mezcla por píxel repetidos. Coste adicional fijo:
187200 bytes. Conserva la imagen completa para exportar sin perder datos.
Cachea índices/paletas por contenedor visitado para no volver a recorrer
sus cabeceras. Registra dat_view_index scan_us/cache_hit y
dat_view_timing decode_us/preview_us/tamaños.

La primera lectura y decodificación siguen siendo síncronas. La
exportación de Start también. No se garantiza una velocidad concreta:
estas métricas permitirán decidir si sigue siendo necesario preparar
miniaturas en PC o llevar la carga a un hilo con precarga.

La consulta observada GAMECTRL/ATTACHEDONLY devuelve S_OK y cero
dispositivos: este puente implementa teclado mediante captura nativa del
pad, no una interfaz joystick DirectInput. Por tanto no invoca callback.
Otros filtros mantienen una frontera explícita. No se modifica el EXE
ni se simula un joystick inexistente. La siguiente frontera se conocerá
en hardware; aún no hay Draw/Present ni pantalla original jugable.

## Entrega

VitaSDK compiló sin advertencias. CRC ZIP, SFO 01.63/T075VITA1 y cuatro
PNG indexados de 8 bits sin entrelazado inspeccionados.
Build ID: iteration59-fast-dat-r1.
VPK: artifacts/iteration59/Touhou75Vita-iteration59.vpk.
SHA256: 0a302c5ad124c629e1a82ee2a19622f5eeba85b8c0767fdd22eb19eda936d804.

Controles: Circle siguiente DAT; izquierda/derecha fotograma; Start
exportación; Triangle música aleatoria; Square reinicio; X salida.
Devolver iteration59.log, iteration59-runtime.log,
iteration59-watchdog.log, iteration59-bgm.log, log.txt y foto.
Comparar respuesta al cambiar imagen, revisitar un contenedor y exportar;
revisar tiempos y preparaciones tardías de música. Ver roadmap detallado
en INFORME_ROADMAP_ITERACION59.md.
