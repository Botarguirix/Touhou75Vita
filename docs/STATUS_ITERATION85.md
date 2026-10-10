# Iteración 85 — Stream Offsets

## Resultado físico de la 84

239 Present reales y 757 dibujos, incluidos seis strips convexos nuevos,
1660196 píxeles en triángulos y 113921828 píxeles cubiertos en total. Se mostró
otra imagen del opening: el santuario y una leyenda japonesa. Raster/hashes:
63,279 s; triángulos: 3,629 s; ejecución: 104,133 s. Se preservaron los 240
contextos de espera main y 221 contextos de despacho cooperativo de audio.
Memoria gráfica usada/pico: 42896712 / 67108864 bytes; cierre y watchdog válidos.

La parada fue el presupuesto de 240 reanudaciones en la espera infinita
original WaitForSingleObject, EIP 00B003C0. La primera espera no genera Present:
se alcanzaron 239 imágenes, no una nueva API sin implementar. No hubo fallo
ni límite temporal. Los primeros 215 scanout y 593 hashes de dibujo coinciden
con 83. Desde el dibujo 594 el EXE envía coordenadas distintas: X -19,0383 en
83 y -21,8174 en 84. Por tanto, no se exige igualdad de todos los hashes
posteriores entre corridas con distinta geometría animada. La relación con
los tiempos del host es una hipótesis; estos logs sí demuestran entradas
geométricas diferentes, no una regresión del renderer por sí solos.

Audio: un Play, 60 uploads activos, 2170 bloques de 1024 frames a 48 kHz
(46,293 s enviados) y 2282054 samples no cero. Máximo intervalo de entrega:
22131 us; ningún intervalo superó dos bloques. Bloqueo de salida máximo:
20189 us; mutex máximo: 2 us; 1048 bloques silenciosos (48,3 %); ninguno
sin voces activas. RC de output/drain/join/delete/release: 0. El usuario
confirmó que la música seguía avanzando, pero entrecortada cuando aumentaba
la carga de CPU. No se registra como reproducción ininterrumpida confirmada.

Después del despacho inicial, la espera del worker de audio llegó a 687872 us,
con mediana 141855,5 us frente a 80000 us pedidos. El primer wake incluye
49,648 s de inicialización previa a Play; no es una interrupción audible de
esa duración. La salida nativa recibió bloques regularmente en esta corrida;
ese dato no determina todas las causas de los cortes físicos. El raster
sigue dominando el tiempo medido y cada dibujo nativo es no preemptible.
El 1 FPS del overlay al final tampoco mide el FPS de toda la ejecución.
Los logs, foto y resumen quedan fuera de Git en artifacts/iteration85/input.

## Error identificado y corrección

En 53 reposiciones, el EXE llamó Lock con ENTIREBUFFER (2), posición distinta
de cero y petición de 128 KiB. El puente anterior ignoraba esa posición y
devolvía siempre el inicio del anillo. Así las reposiciones sobrescribían el
primer tramo; otros tramos podían conservar silencio o datos anteriores.
El EXE en 00408EBC–00408EE6 pasa flags 2 y su posición, y en 00409010–0040902B
lee directamente en el primer puntero devuelto. El extracto local queda
fuera de Git; no se copia la música ni se modifica la lógica del EXE.

ENTIREBUFFER ignora el número de bytes pedido, pero conserva el offset.
La posición se ignora con FROMWRITECURSOR. La 85 conserva el offset y devuelve
los dos tramos del anillo cuando hay wrap, con sus tamaños y punteros reales.
Fuente del contrato: [Microsoft, IDirectSoundBuffer::Lock](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/mt708932(v=vs.85)).
La implementación es propia; no se copió código de Wine ni otro proyecto.

Se mantienen los perfiles restringidos (flags 0/2, PCM y alineación conocidos),
Unlock exacto observado, ownership, commit de cursor, épocas de seek/Stop,
resampling y cola nativa. No se cambia prioridad, ganancia ni frecuencia.
El log de Lock agrega offset, flags, bytes solicitados y ambos punteros para
comprobar la colocación real. No se añade escaneo por bloque al hilo de audio.
La corrección tiene pruebas locales; sus efectos audibles siguen pendientes.

## Continuación acotada del EXE

Hasta 360 Present y 361 reanudaciones, incluyendo la espera inicial. Máximo
180 s y watchdog 210 s. Se permiten 6144 dibujos y 768 Mi píxeles cubiertos;
la escena ha usado hasta 28 dibujos/frame y el límite temporal puede detener
la corrida antes. El presupuesto gráfico independiente continúa en 64 MiB,
1024 despachos audio entre llamadas y máximo 64 slices de CPU.

Agotamiento de reanudaciones se identifica como resume_scope_limit sólo
si el main sigue en la espera exacta original y el productor/estado validan.
La parada de Present tiene su contador propio. No se inventan señales ni
se adelantan tiempos, escenas, menú o llamadas no soportadas. El scheduler
sigue despachando el timer y refill originales después de sus timeouts reales,
con preservación de registros/flags/FS/FPU/callframe/LastError.

LiveArea se vuelve a decodificar desde frame 6 de data/system/selectchar.dat
(Alice) en el th075.dat del usuario. Se compara el RGBA original con la lectura
previa; los assets y su procedencia permanecen fuera de Git.

## Comprobaciones y paquete

80 grupos portables: 16 PCM, ocho mixer y los grupos previos de renderer,
servicios, strings y ownership. Tres grupos PCM nuevos verifican los ocho
tramos de 128 KiB en un anillo de 1 MiB, bytes solicitados ignorados, spans
al envolver, conservación de tramos no escritos, rechazo previo a escrituras,
y captura nativa simulada de distintos tramos/seam sin samples silenciosos.
La regresión nueva falla con el header original de 84 y pasa con el de 85.

ASan/UBSan y -Werror pasaron. Se conservan el replay local de 1312 llamadas,
200000 comparaciones bilineales, 500 strips/oráculo independiente y digests
POINT/LINEAR/triángulos idénticos a -O0/-O2. No son pruebas del EXE en Windows
ni evidencia de audio físico continuo. Precisión contra Windows pendiente.

VitaSDK y validaciones ZIP/CRC, SFO, cuatro PNG indexados de ocho bits,
decodificación completa, segmentos/imports/compresión SELF y límites del
paquete comprobados antes de entregar. No se incluyen EXE/DAT/WAV/log.
Build: iteration85-stream-offsets-r1. Versión 01.89, T075VITA1.
VPK local: artifacts/iteration85/Touhou75Vita-iteration85.vpk.
SHA256: beeddb5569c54c06056ca7836c7009951f22c3cc72eb307690f979b500937db7.

## Próxima corrida física

Instalar 85/01.89, verificar identidad y Alice. Mantener EXE y DAT originales.
Esperar al resultado: límite de ejecución 180 s, watchdog 210 s. Un dibujo
nativo en curso puede terminar después del límite normal. X para salir.
Enviar iteration85.log, iteration85-runtime.log, iteration85-watchdog.log,
log.txt y captura. Anotar si la música avanza sin cortes, reinicia o guarda
silencios y si coincide con carga CPU. El PCM termina al parar el diagnóstico.

Comprobar offset/punteros/spans de Lock, cambios de cursor y lecturas PCM,
silencio/intervalos de salida, nueva escena o API, Present y esperas por
separado, memoria/contextos/cierre. Si el raster sigue dominando, la siguiente
mejora de rendimiento requiere un backend GPU que conserve filtrado, alpha,
estado y ownership. Menú, controles y combate siguen sin confirmar. Las
referencias/decompiladores investigados se conservan en
[investigación](RESEARCH_EXE_ITERATION73.md), sin asumir un motor TH075 completo.
