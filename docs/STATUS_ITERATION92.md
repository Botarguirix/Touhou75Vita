# Iteración 92 — transparencia y mezcla por CPU

## Punto de partida: física 91

El menú original completó 360/360 Present, sin excepción y con watchdog
disarmed. La captura marca 9 FPS. El tramo bajó de 142,44 s en la 90 a
47,83 s en la 91; el ciclo medio fue 118,52 ms (inverso: 8,44 frames/s).
CPU/bus reales: 500/222 MHz, valores existentes del plugin. La petición de
444 MHz no incrementó ese CPU. No se ha confirmado un combate completo.

Todavía se consumieron 23,91 s en 7835 dibujos CPU: 13,32 s POINT y 10,59 s
LINEAR. Hubo 3055 llamadas LINEAR sin píxeles escritos, con 4,91 millones
de muestras rechazadas que costaron 6,13 s. [Evidencia física 91](STATUS_ITERATION91.md).

## Cambios de la 92

1. Descarte temprano de alpha: se calcula el máximo alpha de los vecinos
   POINT/LINEAR y su modulación por el alpha diffuse. Si incluso ese máximo
   queda por debajo de ALPHAREF, el píxel se descarta antes de interpolar RGB,
   modular sus canales o leer el destino. El filtro usa pesos convexos; su
   alpha no puede superar el máximo, y la modulación/redondeo es monótona.
   La comparación sigue siendo GREATEREQUAL. Con test desactivado o ref=0
   no se hace el descarte. No cambia el resultado visible de la 91.
2. Mezcla SRCALPHA/INVSRCALPHA con canales R/B y A/G empaquetados en dos
   carriles de 16 bits. El numerador de cada canal no supera 65152; la división
   exacta por 255 no produce carry entre carriles. También se reduce el coste
   de MODULATE, incluyendo el caso frecuente de RGB diffuse blanco. Conserva
   los cuatro canales, incluido el alpha de destino y la normalización X8.
3. El ejecutor serial rápido omite probes de píxel que el log compacto ya
   descartaba. La API de diagnóstico completa conserva hashes/probes por
   defecto y usa el muestreo completo. Los contadores de coverage, rechazo,
   escritura, cambio y clase de filtro se conservan. Se registra por dibujo
   alpha_shortcuts y al final startup_d3d8_alpha_summary, para medir en Vita
   cuánto trabajo realmente se evitó.

Se conserva 320×240 nativo, el ABI guest 640×480 y sus locks/descriptores,
DAT sin reducción, menú sin intro, límites de 360 Present / 361 esperas /
180 s, audio, controles, clocks y VM/JIT. No hay nuevos workers ni backend
GPU en la 92. La pérdida de detalle pertenece al perfil reducido de la 91;
esta optimización de canales no añade otra reducción ni compresión.

## Comprobaciones locales

- **131 grupos C++ y 5 pruebas Python portables** con ASan/UBSan y -Werror; audio/input/TLS,
  ownership de 1312 llamadas, geometría/alpha/filtros/locks/scanout y clocks.
- Seis grupos nuevos: división exacta en todo el rango 0..65152 y ambos
  carriles; 400.000 pares de modulación/mezcla contra un oracle escalar;
  extremos por alpha/canal; 384 replays de rectángulos y quads convexos
  con pitches y formatos; alpha transparente coloreado/X8/diagnósticos;
  test desactivado/ref=0; rechazos antes de escribir.
- Un grupo COM nuevo llama DrawPrimitiveUP del D3D8Storage que se compila
  para Vita: POINT y LINEAR transparentes, 153.600 shortcuts, readback guest
  640×480 idéntico y resumen real, sin probes.
- Misma salida y digests a -O0/-O2. El replay también se compiló con el
  header exacto del commit físico 91 c3ac4fa: digest B7F123D8 y salidas iguales.
  Los digests de logo, fade, LINEAR y triángulos anteriores siguen iguales.
- Build VitaSDK -O2 / FP estricto; pthread_cancel/once/cond_destroy fuertes,
  sin pool raster ni pthread débiles. SELF/segmentos/memoria/imports, PNG8,
  ZIP/CRC, SFO y cuatro fingerprints del EXE revisados.

### Microbenchmark comparativo

Mismo programa, texturas sintéticas, 320×240 y 64 draws por caso; la referencia
usa el header de la 91. Cada caso calcula 4.915.200 píxeles. Se comprobaron
los mismos digests de destino, incluyendo alpha/X8.

| Caso CPU host | 91 | 92 | Ratio de tiempos 91/92 |
|---|---:|---:|---:|
| POINT con alpha mixto | 117.022 µs | 62.111 µs | 1,88 |
| LINEAR transparente con RGB | 184.956 µs | 54.641 µs | 3,38 |
| LINEAR con alpha mixto | 246.141 µs | 189.866 µs | 1,30 |

Esto mide CPU de PC, sin EXE/DAT reales, GPU, audio, scheduler ni logs.
**No equivale a estos factores de FPS en Vita.** La prueba física 92 está
pendiente; comparar tiempos y shortcuts con la 91 usando el mismo menú,
clocks y plugins. Las cifras de una ejecución host pueden variar.

## Objetivo de 60 FPS

Se añade FrameBudget al loop real: cuenta solo ciclos que producen exactamente
un Present confirmado y compara cada duración con 16.667 µs. Registra frames
cumplidos/fallidos y percentiles p50/p95/p99 en startup_60fps_budget. Reutiliza
la medición de tiempo existente; no altera pacing, input ni lógica del juego.
Seis grupos C++ verifican límites, exclusiones, cuantiles, almacenamiento y log.
Cinco pruebas Python verifican el analizador reproducible de registros.

El análisis del menú físico 91 (337 ciclos con Present) da media 114,57 ms,
p95 165,64 ms y 0/337 dentro de 16,7 ms: raster 65,34 ms, Present 16,49 ms,
resto 32,74 ms. Ese resto incluye guest/otras APIs/scheduler/logs; no es una
medición pura de JIT. La GPU sola no demuestra llegar a 60: también se debe
reducir ese coste restante. [Recursos PC y plan de 60 FPS](PERFORMANCE_60FPS.md).

## Paquete

- VPK local: artifacts/iteration92/Touhou75Vita-iteration92.vpk.
- Title ID T075VITA1; versión **01.96**.
- Nombre: **Touhou 7.5 Vita - Iteration 92 Alpha Fast Path**.
- Build: **iteration92-alpha-fast-path-r1**; 1803643 bytes.
- SHA-256: 220fa7fea313f23f3d3958d98478ab1136e90bddc3a7ab89d7f0be1bcf903b27
- LiveArea revisión 92, codificada desde los maestros originales. Los cuatro
  PNG conservan los mismos bytes que la 91; cambia la identidad de paquete.

## Comprobación física y siguientes pasos

1. Instalar la 92 y confirmar identidad/build, preflight, menú y source:320x240.
2. Comparar startup_elapsed_us, suma draw_elapsed_us y ciclo medio con la 91.
   Recoger alpha_shortcuts por draw, startup_d3d8_alpha_summary y startup_60fps_budget.
3. Revisar texto/alpha/bordes y navegación con el mismo perfil de CPU/plugins.
4. Completar 360 Present o guardar importación/límite/core si se detiene antes.
5. Adjuntar iteration92.log, iteration92-runtime.log, iteration92-watchdog.log,
   log.txt y captura con FPS. La apertura musical se omite por el preset de menú;
   la falta de esa pista no es por sí sola un nuevo fallo de audio.

Después, medir el coste restante del guest/JIT y preparar quads/texturas GXM
con validación de alpha, filtro, blend, orden, uploads y ownership. El renderer
CPU queda como referencia de píxeles y fallback. Cualquier nuevo pool deberá
usar contexto pthread registrado o sincronización kernel y validación física
antes de activarse; no se reactiva la ruta que falló en 87–89.

## Resultado físico recibido

360 Present, 49,215 s total, 19,421 s raster y fin por 361 esperas. Sin excepción;
337 ciclos menú a 121,707 ms/8,216 frames/s. Capturas 7–10 FPS instantáneos.
Usuario: Cross en opciones produce negro y diagnóstico. Este log sólo registra
flechas y logo/título; no demuestra esa transición. [Seguimiento 93](STATUS_ITERATION93.md).
