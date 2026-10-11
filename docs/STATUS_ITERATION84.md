# Iteración 84 — Triangle Strips

## Evidencia física de la 83

228 Present reales, 695 dibujos y una nueva imagen del opening, marrón y con
texto japonés. Los primeros 208 hashes scanout y 397 hashes de dibujo coinciden
con 82. Los mismos 397 dibujos consumieron 43,172 s en 83 frente a 68,308 s;
es una medición de dos corridas. El raster/hashes total consumió 60,435 s de
99,672 s de ejecución. Se preservaron 230 contextos de espera main y 189 de
despacho audio. Memoria gráfica: 42896712 / 67108864 bytes; watchdog desarmado.

La parada fue DrawPrimitiveUP número 696, textura 00ABCAB8, strip 5 con dos
primitivas, FVF 144/stride 28, LINEAR/WRAP. Coordenadas impresas aproximadas:

| Vértice | X | Y | U | V |
|---|---:|---:|---:|---:|
| 0 | -0.504439 | 142.673 | 0 | 0 |
| 1 | 639.503 | 142.675 | .625 | 0 |
| 2 | -0.505992 | 654.679 | 0 | 1 |
| 3 | 639.501 | 654.681 | .625 | 1 |

Z .5, RHW 1 y diffuse FFFFFFFF constantes. Es ligeramente inclinado:
el contrato rectangular lo rechazó sin escribir. Los bits float originales
no se imprimían; esta tabla no permite reconstruir exactamente esos bits.

Audio: un Play, 44 uploads PCM activos, 1613 bloques de 1024 frames a 48 kHz,
34,411 s enviados, 1711468 samples no cero, output/drain/join/delete/release
RC 0. El usuario escuchó algo más de música, todavía entrecortada, y observó
CPU al máximo. Tras el primer wake, mediana 138064 us y máximo 508982 us,
frente a 80000 us pedidos. El primer wake de 55,094 s incluye inicialización
previa a Play: no describe una interrupción audible de esa duración.

El anillo de 1 MiB cubre aproximadamente 5,94 s de PCM estéreo/16 bits a
44100 Hz. Los retrasos del worker no bastan para demostrar la causa de todos
los cortes; faltan tiempos entre entregas a la salida nativa. El 1 FPS del
overlay en la pantalla final tampoco establece el FPS durante el EXE.
Logs/foto completos y resumen quedan fuera de Git en artifacts/iteration84.

## Cambios de la 84

1. Rasterización acotada de strips convexos de cuatro vértices mediante
   triángulos (0,1,2) y (2,1,3). RHW 1, Z/diffuse constantes y perfiles D3D8
   existentes siguen siendo obligatorios. No se ajustan los vértices a un
   rectángulo ni se consume una llamada sin renderizarla.
2. Centros de píxel enteros, cobertura top-left y diagonal compartida con
   extremos canónicos: la misma arista se evalúa con signos opuestos. UV
   afines por triángulo; sin tolerancias geométricas. Triángulos de winding
   inverso se normalizan porque el perfil observado desactiva culling.
3. El muestreo POINT/LINEAR WRAP, MODULATE, alpha GE y SRCALPHA/INVSRCALPHA
   o ONE/ZERO comparten el código previo. Hashes conservan orden por filas.
   Rectángulos exactos con UV separables mantienen sus tablas optimizadas.
   El helper de píxel se fuerza inline en GCC: la primera inspección ARM
   detectó que -O2 lo separaba en una llamada por píxel tras el refactor.
4. Concavidad, strip plegado, degeneración, perspectiva, diffuse variable,
   alias, formatos y perfiles ajenos siguen rechazándose antes de escribir.
   Se imprimen los siete DWORDs originales de cada vértice, junto al hash.
   Esto permite replay exacto de geometría en futuras corridas.
5. Audio agrega intervalos entre submissions, duración del bloqueo nativo,
   espera de mutex y bloques sin voces/en silencio. Se resumen tras join;
   no se escribe al log desde cada bloque del hilo de audio. Son mediciones
   del reloj host, no un contador de underruns del hardware. Lateness luego
   del primer despacho se registra aparte. El algoritmo PCM no se modifica.
6. La frontera llegó a 99,672 s, casi al límite anterior. Se permiten 130 s
   y watchdog 150 s para ejercitarla; se conservan 240 Present/waits, 2048
   dibujos, 384 Mi píxeles, 1024 despachos audio, 64 slices y 64 MiB gráficos.
7. LiveArea vuelve a partir del frame 6 original de selectchar.dat (Alice),
   leído de th075.dat. No parte de una portada ya procesada; los assets,
   procedencia y datos del usuario permanecen fuera de Git.

La implementación usa double para aristas/interpolación y el redondeo del
modelo anterior para LINEAR. La precisión de filtros y subpíxeles del D3D
original puede diferir. Referencia de cobertura: [Microsoft, reglas de
rasterización](https://learn.microsoft.com/en-us/windows/win32/direct3d9/rasterization-rules).
No se copió código de esa documentación. Comparación contra Windows pendiente.

## Comprobaciones y paquete

77 grupos portables a -O2, -Werror y ASan/UBSan: nueve grupos nuevos de strip
más 500 casos deterministas con ambos windings/UV/clipping y un grupo COM
de la clase enviada. El oráculo independiente usa long double: cobertura
exacta y colores dentro de una unidad por canal, por precisión/redondeo.
La geometría física 83 se prueba con las coordenadas redondeadas disponibles,
no con un replay exacto. Se conservan 200000 comparaciones bilineales y el
replay local de 1312 llamadas de ownership. POINT/LINEAR mantienen sus digests
anteriores; las salidas de los tres caminos coinciden a -O0 y -O2.

VitaSDK compiló con -O2 y FP estricto. ZIP/CRC, SFO, cuatro PNG indexados de
ocho bits y decodificación completa, segmentos/imports/compresión SELF y
límites de memoria del paquete comprobados. El VPK no incluye EXE/DAT/WAV/log.

Build: iteration84-triangle-strips-r1. Versión 01.88, T075VITA1.
VPK local: artifacts/iteration84/Touhou75Vita-iteration84.vpk.
SHA256: da6c33b98f761dcdcfa132068b19be1f866c85e5e3349af896f0fe3703c5e90d.

## Próxima corrida física

Instalar 84/01.88, comprobar identidad y Alice en LiveArea. Mantener los DAT
y EXE; esperar al resultado (hasta unos 130 s, watchdog 150 s), X para salir.
Enviar iteration84.log, iteration84-runtime.log, iteration84-watchdog.log,
log.txt y captura. Anotar duración/cortes del audio y cuándo se eleva CPU.

Verificar dibujos convex_triangle_strip, DWORDs sin pérdida, 228 hashes previos,
nuevos Present/escena, contextos preservados, tiempos de dibujo y salida audio,
cursores/uploads, siguiente frontera, memoria/shutdown y watchdog. Llegar al
límite de 240 frames sigue siendo una comprobación acotada. Menú, controles,
audio continuo y juego completo aún no están confirmados.

## Resultado físico de la 84 (10 de octubre de 2026)

239 Present y 757 dibujos, incluidos seis strips convexos. Raster/hashes:
63,279 s de 104,133 s. 240 contextos main y 221 de despacho audio preservados;
memoria gráfica 42896712 / 67108864 bytes. Paró al agotar 240 reanudaciones
(la primera no presenta), en la espera infinita original. Otra imagen del
opening visible. Los primeros 215 scanout y 593 hashes de dibujo coinciden
con 83; desde el dibujo 594 el EXE envía coordenadas animadas diferentes.

El usuario confirma progreso musical, pero entrecortado coincidiendo con
carga CPU. 2170 bloques, 1048 silenciosos, máximo intervalo nativo 22131 us,
cero intervalos mayores a dos bloques. Se halló un error del puente: 53 Lock
ENTIREBUFFER con offset no cero se colocaron siempre al inicio del anillo.
Corrección y próxima corrida: [iteración 85](STATUS_ITERATION85.md).
