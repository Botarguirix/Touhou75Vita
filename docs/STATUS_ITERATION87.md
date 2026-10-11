# Iteración 87 — Parallel Raster

## Confirmado en la Vita: iteración 86

El usuario alcanzó el menú original, navegó con los controles y escuchó los
efectos limpios. Los scans entregados tienen pulsación y liberación; el campo
selection del TitleScene original recorre 0..9. Hay 250 observaciones de ese
menú, 272 Present originales, 273 esperas reanudadas y 5811 dibujos.
La corrida terminó por time_cap a 180,484 s, no por una API nueva ni CPU fault.
El salto de la 86 conserva el fade breve del logo: no desaparece todo el arranque.

154648700 píxeles cubiertos. Dibujo/hashes: 88,729 s (49,2 % del tiempo);
Present/conversión/captura/espera de display: 54,769 s adicionales. Ese segundo
valor incluye espera, no mide sólo trabajo de CPU. No se identifica todo coste
con GPU ni se interpreta un porcentaje del overlay como carga global.
El runner estaba fijado a USER_0 (00010000); renderer software sin GXM.
El log principal pesa 34458216 bytes, con estado repetido por dibujo.
Memoria gráfica final y pico 30362440 bytes (29,0 MiB), límite 64 MiB.

Audio: 4738 bloques, 166120 samples no cero, 4650 bloques silenciosos y también
sin voces activas. Hay 88 bloques activos; los silencios largos concuerdan
con el opening omitido. Un intervalo nativo de 118469 us supera dos bloques;
eso no demuestra ausencia total de underruns. El usuario confirmó sonidos
limpios en esta corrida; no se da por probado todo el audio del combate.
Contextos, cierre y watchdog se conservan. Combate jugable pendiente.
Logs, foto y resumen físico quedan fuera de Git.

## Cambios enviados en la 87

| Trabajo | Distribución |
|---|---|
| EXE x86/JIT, servicios, entradas, recursos y dibujos pequeños | Main, USER_0. |
| Primer tercio de filas de dibujos grandes | Hilo nativo USER_1. |
| Segundo tercio | Hilo nativo USER_2. |
| Último tercio | Main, USER_0. |
| Audio PCM nativo | Pipeline existente, prioridad superior a workers. |
| Watchdog | Hilo existente, USER_1, prioridad superior a worker. |

Sólo se paralelizan dibujos con bbox visible de al menos 32768 píxeles y
tres bandas no vacías. Dos hilos persistentes, pila 128 KiB por hilo,
prioridad 10000110 (inferior a 10000100 del audio/watchdog). Se comprueba
afinidad y núcleo efectivo. Fallos de creación/start/afinidad cierran los
hilos creados y usan el renderer serial; no se declara paralelismo inexistente.

Los workers duermen con condition variables entre trabajos. No ejecutan el
EXE, no comparten el JIT ni llaman al guest. Cada dibujo valida dimensiones,
almacenamiento, alias, vértices, convexidad y ajustes antes de cualquier
escritura. Textura y vértices inmutables, filas de destino disjuntas; main
espera a ambos workers antes de liberar/modificar un recurso, dibujar otra
capa o presentar. Se conserva el orden del EXE, blending, alpha, WRAP,
POINT/LINEAR y regla top-left, sin redondear geometría. Contadores y probes
se combinan en orden de filas. No se permite liberar un hilo sin join.

Se omiten los tres hashes de diagnóstico por píxel y el hash del scanout.
Estos hashes no determinaban igualdad del caché: se mantiene memcmp de
todos los bytes y confirmación real de display/buffer. La conversión y los
píxeles son los mismos. El log marca hashes disabled, nunca publica ceros
como hashes válidos. No se concatenan FNV parciales incorrectamente.
Los dumps completos de estado se conservan al encontrar una frontera;
se omite repetirlos en cada dibujo soportado. Vértices/probes/costes siguen.

GPU aún no integrada; no se solicitan overclock ni el núcleo de sistema.
Cambiar afinidad de un solo hilo no lo ejecutaría en paralelo. Véase
[investigación y ruta GPU](PERFORMANCE_ITERATION87.md).

## Comprobaciones

106 grupos portables: 96 previos, uno de scanout sin hash y nueve del pool
con hilos host reales. -Werror, ASan/UBSan. Se comparan bytes completos,
contadores y probes contra el renderer serial: rectángulos y 48 strips
afines sucesivos, POINT/LINEAR, A8/A1/X8, alpha, WRAP, clipping y pitch.
Se cubren dry validation, casos pequeños/vacíos, alias/vértices inválidos,
fallos en ambos create/start/afinidad y join/delete de cada hilo iniciado.
Son simulaciones de API nativa; no prueban scheduling o FPS físicos.

200000 comparaciones bilineales, 500 strips/oráculo, replay de 1312 llamadas,
digests POINT/LINEAR/triángulos a -O0/-O2 y los siete grupos del preset
siguen pasando. Los hashes se conservan habilitados en el renderer de
referencia para estas comprobaciones. Cuatro fingerprints del EXE original
validados; el mismo salto al menú ya quedó confirmado físicamente en la 86.
VitaSDK, ZIP/CRC, SFO, PNG completos y SELF/imports/memoria comprobados.
LiveArea: Alice recién decodificada del DAT; arte/EXE/DAT/logs fuera de Git.
EXE/DAT/logs no se empaquetan. Imagen de controles generada con image_gen,
correspondencias revisadas, fuera del paquete de ejecución.

## Paquete y prueba física

VPK: artifacts/iteration87/Touhou75Vita-iteration87.vpk.
Versión 01.91, T075VITA1, build iteration87-parallel-raster-r1.
SHA256: 0ce89b3792f69fec75ae090d8b820d391fbd7496a9bb0e84e64baeea547b2b3e.

Límites: 360 Present / 361 esperas / 180 s, watchdog 210 s. Dibujos suben
de 6144 a 12288 para permitir 360 frames a la carga observada; no se inventan
frames. Se conservan 768 Mi píxeles / 64 MiB gráficos / 1024 despachos audio.
La corrida puede terminar antes por una frontera o cualquiera de sus límites.

Instalar 87/01.91 y probar las mismas opciones del menú. Enviar iteration87.log,
iteration87-runtime.log, iteration87-watchdog.log, log.txt y captura.
Revisar startup_raster_worker verified:yes, actual_cpu 1/2, cantidad de dibujos
paralelos/seriales, joins/deletes, tiempo por dibujo/Present, entrada, memoria
y audio. Si hay fallback:serial, diagnosticar ese motivo con sus rc.
La mejora de FPS/tiempo y limpieza del audio bajo esta carga requieren Vita.
Los otros núcleos pueden mostrar más carga al repartir trabajo; importa
cuánto tarda cada frame, no que todos los porcentajes bajen.
