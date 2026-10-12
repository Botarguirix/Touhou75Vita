# Comprobaciones — iteración 92

## Local: completado

- Física 91: menú, 360 Present, sin excepción; 47,83 s total / 23,91 s dibujo.
- 131 grupos C++ y 5 pruebas Python ASan/UBSan; mezcla escalar/empaquetada, alpha, COM/readback,
  audio/input/TLS, geometría, ownership y perfil reducido.
- Replay idéntico al commit 91; microbenchmarks sintéticos con mismos digests.
- Build VitaSDK, SELF/imports/memoria, SFO/PNG8, ZIP/CRC y fingerprints EXE.

## Vita: pendiente

1. Identidad iteration92-alpha-fast-path-r1 / 01.96.
2. Menú, serial_half_resolution, pixel_probes:disabled y source:320x240.
3. Comparar tiempo de dibujo y ciclo/FPS con la 91, mismos clocks/plugins.
4. Recoger startup_d3d8_alpha_summary, alpha_shortcuts por dibujo y startup_60fps_budget (meta 16.667 µs/frame).
5. Revisar alpha/textos/bordes, navegación y 360 Present sin excepción.
6. Selección de personaje y siguientes servicios/combate todavía pendientes.

Adjuntar iteration92.log, iteration92-runtime.log, iteration92-watchdog.log,
log.txt y captura. Preservar el core si se produce una excepción.
