# Comprobaciones — iteración 90

## Verificado localmente

- Comparación de commits desde 86; runtime, audio, input y preset no cambiaron.
- Core completo 89: worker nativo sin contexto OS pthread, fallo en espera cancelable.
- La 89 supera preflight/VM/JIT e inicialización original antes del fallo.
- Ruta serial 90 igual en píxeles/counters/probes y sin creación de workers.
- ELF/SELF sin RasterWorkers; 110 grupos host y paquete/fingerprints correctos.
- LiveArea desde maestros originales; versión 01.94 / revisión 90.

## Comprobar en Vita

1. Identidad iteration90-serial-recovery-r1 y versión 01.94.
2. Preflight y VM/JIT completados; startup_raster_policy=serial_recovery.
3. Al menos un Present y menú visible sin C2-12828-1.
4. Navegación y efectos comparados con la 86; sin creación de workers de raster.
5. Tiempos de dibujo/Present y carga CPU medidos; GPU y pool corregido pendientes.
6. Selección de personaje/combate y siguientes límites reales del EXE.

Recoger iteration90.log, iteration90-runtime.log, iteration90-watchdog.log,
log.txt y captura; ante excepción, conservar el nuevo psp2dmp/.tmp.
