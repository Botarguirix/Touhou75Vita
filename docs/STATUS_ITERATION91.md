# Iteración 91 — resolución interna y rendimiento

## Resultado físico de la 91 recibido el 11 de octubre

La captura muestra menú original, 360/360 Present y un indicador de 9 FPS.
Los logs confirman watchdog disarmed, sin excepción nativa ni límite de CPU:
finaliza en la espera prevista de WaitForSingleObject, tras 361 resumes.

| Medida | Física 90 | Física 91 |
|---|---:|---:|
| Tramo de arranque | 142,44 s | 47,83 s |
| Dibujo CPU, 7835 llamadas | 86,69 s | 23,91 s |
| Present, 360 frames | 15,15 s | 6,73 s |
| Ciclo medio de frame | 369,52 ms | 118,52 ms |
| Diagnóstico | 31,99 MB | 5,37 MB |

La duración total se redujo a aproximadamente un tercio. Los alcances se
solapan: frame incluye raster/Present/esperas y no se suma al tiempo de dibujo.
El inverso del ciclo medio equivale a 8,44 frames/s en este tramo; la captura
marca 9, sin demostrar un framerate de combate. La lectura real fue CPU 500 /
bus 222 / GPU 222 / xbar 166 MHz: el floor no aumentó el clock existente del
plugin. No se atribuye la mejora a una subida de frecuencia.

Gráficos usados: 31.718.216 bytes de 64 MiB; quedan 60 MiB user y 108 MiB
CDRAM libres al terminar. Se observaron 3055 dibujos LINEAR sin ninguna
escritura: sus 4.908.800 muestras transparentes consumieron 6,13 s. Los 1075
POINT consumieron 13,32 s y rechazaron 17.449.844 píxeles por alpha. Esto guía
la optimización exacta de transparencia y mezcla de la iteración 92.

En esta 91 no hay dispatches del worker original de audio; el preset de menú
omite la pista de apertura. Los registros no confirman nueva reproducción
musical, controles físicos de esta versión ni combate.

## Base física confirmada: 90

Los logs y captura recibidos el 11 de octubre de 2026 muestran menú original,
360 Present de 360 y watchdog disarmed, sin excepción nativa. El final es el
límite esperado de 361 esperas, no un nuevo fallo de arranque. Se mantienen
el EXE y el preset de menú; no se ha confirmado todavía un combate completo.

| Medida del log 90 | Resultado |
|---|---:|
| Duración del tramo de arranque | 142,44 s |
| Dibujos por CPU | 7835 llamadas; 86,69 s acumulados |
| Present, conversión, captura y espera nativa | 360; 15,15 s acumulados |
| Ciclos de frame | 369,52 ms de media, incluyendo esperas |
| Filtros de dibujo | 1075 POINT / 6760 LINEAR |
| Archivo de diagnóstico | 31.987.307 bytes; 561.786 líneas |

Los tiempos tienen alcances distintos: Present incluye VBlank, y frame
incluye dibujo y scheduler. No deben sumarse como categorías independientes.
360 / 142,44 equivale a 2,53 frames/s para este tramo, incluida inicialización;
no es una medición de FPS de combate. RAM gráfica: 30.362.440 bytes de 64 MiB;
había unos 60 MiB libres de user y 108 MiB de CDRAM al finalizar. La evidencia
apunta al dibujo por CPU y al coste de trazas; no demuestra falta de RAM.

## Cambios de la 91

1. Superficies nativas de color a media resolución: backbuffer 320×240 y
   compañeros reducidos de render targets. La pantalla sigue siendo 960×544
   con proporción 4:3. Calcula 76.800 píxeles en un quad completo, frente a
   307.200; es un perfil visual con pérdida de detalle, no fidelidad idéntica.
2. El EXE conserva 640×480, descriptores, pitch, UVs, staging y ABI originales.
   Solo se transforman coordenadas del dibujo nativo, conservando el offset
   de medio texel. LockRect expande la superficie; UnlockRect actualiza su
   compañero. Targets usados como texturas leen el compañero reducido.
3. DAT/texturas normales conservan resolución y alpha; no se comprimen al
   dibujar. El software necesita texeles decodificados: la compresión runtime
   añadiría cómputo y no sustituye el rasterizador. Los compañeros añaden RAM
   contabilizada en el presupuesto de 64 MiB; esta medida reduce cómputo, no
   pretende reducir toda la memoria de recursos.
4. Al ampliar para scanout, se copian filas ya convertidas cuando el muestreo
   selecciona la misma fila. Se conserva la caché por comparación completa de
   bytes, separando perfiles 640×480 y 320×240.
5. Logs compactos: omiten argumentos, vértices y trazas repetidas exitosas;
   conservan errores, límites, tiempos, allocations y resúmenes. Buffer de
   128 KiB; flush cada 256 callbacks o al detenerse/cambiar de slice. Los
   tests host siguen usando trazas completas por defecto.
6. Consulta y pide como mínimo CPU 444 MHz / bus 222 MHz si están por debajo.
   Respeta valores mayores de plugins, registra rc y lectura real, y restaura
   al terminar solo valores que siguen siendo los solicitados por el probe.
   No cambia clocks GPU porque este paquete aún no tiene renderer GXM.

La ruta de dibujo continúa serial: no se reactiva el pool de la regresión 87.
Audio nativo, controles, VM/Box86 y fingerprints del menú se conservan. No se
añaden esperas C++ en hilos creados con sceKernelCreateThread.

## Comprobaciones

- 118 grupos portables con ASan/UBSan y -Werror. Incluyen dos casos nuevos
  de D3D8Storage real: Clear/LockRect/descriptores/presupuesto y
  DrawPrimitiveUP con textura render target, 76.800 muestras y readback.
- Seis grupos de coordenadas/viewport/dimensiones impares, pitch/alpha,
  POINT/LINEAR, scanout/caché y entradas inválidas, trazas y clocks.
- Referencia 640×480 sin cambios de digests a -O0/-O2; 200.000 muestras
  bilineales, 500 geometrías y 1312 llamadas de ownership. Audio/input/TLS pasan.
- Microbenchmark sintético host: 11059200 frente a 2764800
  píxeles; 531951 µs frente a 135442 µs,
  ratio 3.93. Solo mide CPU de PC, sin EXE, DAT, GPU, audio,
  scheduler o logs; no predice una multiplicación de FPS en Vita.
- Compilación VitaSDK -O2 y FP estricto; SELF, ZIP/CRC, SFO, PNG8,
  controles de memoria e imports comprobados. Los 14 imports previos se
  conservan; se añade ScePower con seis funciones de clocks.
- Arte LiveArea reencodificado desde los mismos maestros originales,
  revisión 91 / versión 01.95; no procede de capturas acumuladas.

## Paquete y prueba física

- VPK: artifacts/iteration91/Touhou75Vita-iteration91.vpk.
- Title ID T075VITA1, versión 01.95.
- Nombre: Touhou 7.5 Vita - Iteration 91 Half Resolution.
- Build: iteration91-half-resolution-r1; 1801070 bytes.
- SHA-256: 41de6deaf77f220ebc5ceadbd98faa5878ac09d076c12cdaabf32faa523c425b

Instalar la 91 y recoger iteration91.log, iteration91-runtime.log,
iteration91-watchdog.log y log.txt, con captura del menú/FPS. Comparar
startup_elapsed_us, draw_elapsed_us, present_elapsed_us y frame_cycle_elapsed_us
con la 90. Confirmar startup_raster_policy=serial_half_resolution,
source:320x240 en Present y lectura real de clocks. Comprobar selección de menú,
texto/alpha, controles y audio. Si falla, conservar el core.

La prueba física de la 91 confirmó menú, 360 Present y reducción del tiempo.
Audio/controles de esta versión y combate siguen pendientes de confirmación.
Las referencias 90 y 91 quedan conservadas para comparar el siguiente paquete.

## Siguientes mejoras de recursos

| Prioridad | Trabajo | Motivo / comprobación |
|---|---|---|
| 1 | Medir la 92 en Vita | Descarte temprano de alpha y mezcla exacta por canales empaquetados |
| 2 | Backend GXM para quads/texturas | Trasladar filtro, modulación y blend a GPU; comparar capturas, alpha y orden de dibujo |
| 3 | Cachear uploads GPU en UnlockRect | Un upload por modificación, mantener ownership y sincronización, evitar readback frecuente |
| 4 | Optimizar scheduler/JIT según nuevos tiempos | El código x86 del juego es secuencial y no se reparte automáticamente entre cores |
| 5 | Pool CPU registrado, si aún hace falta | pthread_create/join o sincronización kernel; validar inicio/TLS/espera/afinidad/teardown en Vita |

GXM es una ruta pendiente, no GPU integrada en la 91. libvita2d proporciona
ejemplos públicos de contexto, targets y colas; todavía debe adaptarse al
perfil D3D8 observado, alpha, filtros, formats y liberación segura. Evitar
comprimir sobre la marcha antes de medir uploads/cache: sprites con alpha
no toleran una conversión JPEG, y un renderer CPU debe descomprimir para leerlos.

Fuentes primarias revisadas:
[ScePower de VitaSDK](https://docs.vitasdk.org/group__ScePowerUser.html),
[petición de 444 MHz en VitaShell](https://github.com/TheOfficialFloW/VitaShell/blob/master/init.c),
[implementación GXM de libvita2d](https://github.com/xerpi/libvita2d/blob/master/libvita2d/source/vita2d.c),
[tipos y funciones GXM](https://docs.vitasdk.org/group__SceGxmUser.html).
