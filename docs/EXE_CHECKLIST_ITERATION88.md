# Comprobaciones — iteración 88

## Resuelto localmente

- Core 87 y ELF coincidente: data abort en mutex nulo durante primer worker.
- Enlace incluye pthread_cancel y llamada real de bloqueo, sin invocar cancel.
- Preflight de activación/mutex antes del EXE; logs de worker vaciados al disco.
- 107 grupos, sanitizadores, renderer serial/paralelo idéntico, audio e input.
- VitaSDK, ZIP/CRC, SFO/PNG, imports/segmentos/memoria SELF y fingerprints.

## Comprobar en Vita

1. Versión 01.92/build iteration88-thread-startup-r1 y arranque sin C2-12828-1.
2. startup_native_thread_support passed; ambos workers verified:yes con core 1/2.
3. startup_raster_parallel_summary con trabajos reales; cierres/join correctos.
4. Menú original, controles y selección; imagen igual a la 86, efectos limpios.
5. Coste real de dibujo/Present y tiempo total; no inferir FPS de tests host.
6. Selección de personaje y combate, servicios/estados aún no implementados.

Entregar iteration88.log, iteration88-runtime.log, iteration88-watchdog.log,
log.txt y captura. Con nueva excepción, entregar también su psp2dmp.
