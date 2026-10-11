# Comprobaciones — iteración 89

## Verificado localmente

- Core 88 parcial con registros de fallo utilizables: mutex nulo dentro del TLS del JIT.
- pthread_cancel/once/cond_destroy definidos; llamada real de libgcc a pthread_once.
- Preflight mutex/once/TLS y TLS por worker; checkpoints vaciados antes del VM/JIT.
- 109 grupos portables, sanitizadores y resultados gráficos serial/paralelo iguales.
- VitaSDK, paquete, PNG8, SFO, SELF, imports y fingerprints del EXE.
- Fondo Reimu/Marisa e icono Reimu desde maestros nuevos; revisión/versión del arte comprobadas.

## Pendiente en Vita

1. Nombre de iteración 89, versión 01.93, nuevo fondo e icono.
2. once/TLS/preflight passed, runtime y smoke JIT completados sin C2-12828-1.
3. Workers de núcleos 1/2 con verified:yes y tls:passed; resumen de trabajos y cierre.
4. Menú original, navegación, controles y efectos limpios, comparados con la 86.
5. Coste de raster/Present, carga por núcleo y tiempo total; medir FPS reales.
6. Selección de personaje, combate y servicios/estados aún sin implementar.

Recoger iteration89.log, iteration89-runtime.log, iteration89-watchdog.log
si existe, log.txt y captura; con excepción, conservar el nuevo psp2dmp/.tmp.
