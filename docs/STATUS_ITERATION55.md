# Iteración 55 — DAT Audio / 01.59

## Evidencia recibida de la 54

DirectSound8 fue creado por COM. Initialize abrió el puerto nativo 7 y
comprobó 1024 frames, 48000 Hz y estéreo. SetCooperativeLevel aceptó la
ventana propia con nivel 2; los tres contratos conservaron la ABI.
El EXE se detuvo en CreateSoundBuffer con descriptor de 36 bytes,
flags 00040001, tamaño 0 y formato nulo: buffer primario.
El puerto se liberó con rc 0; el watchdog quedó desarmado y el arranque
duró 6.158302 segundos. No se enviaron muestras en la 54.

## Recurso original extraído

Se extrajo `wave/se.dat` de `th075.dat` y se decodificó el efecto de índice 2.
Es un contenedor de muestras PCM, no código que necesite descompilarse.
Su función concreta dentro del juego no se ha identificado.

| Propiedad | Valor medido |
| --- | --- |
| Formato | PCM firmado de 16 bits, mono, 44100 Hz |
| Frames / bytes | 20256 / 40512 |
| Duración | 0.45932 segundos |
| Pico absoluto | 32767 |
| Muestras no nulas | 20222 |
| PCM FNV-1a32 esperado en Vita | BC1511DF |
| PCM SHA256 en Windows | ee3ec570cc4fbc774e16d6c54c9b420b1d2971ac768310ef45c1dcdbea40ba5a |

El WAV local conserva las muestras del DAT: se verificó leyendo de nuevo
el archivo generado. Herramienta reproducible: `tools/extract_th075_sound.py`.
El WAV y los datos propietarios permanecen fuera del repositorio y del VPK.

## Prueba audible

La 55 carga ese efecto directamente de `ux0:data/TH075Vita/th075.dat`.
El lector comprueba límites del directorio y de todos los registros del
contenedor, así como formato PCM, duración máxima de dos segundos y
muestras no silenciosas. No necesita copiar un WAV adicional a la consola.

La conversión usa interpolación lineal de 44100 a 48000 Hz y duplica mono
a estéreo para el puerto MAIN de Vita. Tras presentar la pantalla de
resultados, se reproduce una vez automáticamente. Cuadrado repite el
efecto; X sale. La ganancia del puerto es 25% y también se aplica el volumen
que tengas seleccionado en la consola.

El audio se entrega en bloques de 1024 frames con relleno final de silencio.
Cada bloque se mantiene vivo hasta terminar el drenaje documentado; el
puerto se libera al concluir o ante error. No hay reproducción en bucle.

Los registros incluyen `audio_probe_original`, hash, muestras no nulas,
conversión, número de ejecuciones, configuración, resultado de volumen,
bloques entregados, drenaje y liberación. `submitted_and_drained` demuestra
que la API aceptó y drenó las muestras; la audibilidad necesita confirmación
del usuario. No se marca como confirmada por una llamada API exitosa.

Esta es una prueba nativa independiente del camino DirectSound del EXE.
La frontera original sigue en CreateSoundBuffer. No declara implementados
buffers DirectSound, mezcla del juego, uploads gráficos, Draw/Present o menú.

## Comprobación y entrega

VitaSDK compiló sin advertencias. CRC ZIP, APP_VER 01.59, TITLE_ID T075VITA1
y cuatro PNG indexados de 8 bits sin entrelazado comprobados.
Falta ejecutar la 55 en la Vita y confirmar que se escucha el efecto.

Build ID: iteration55-dat-audio-r1.
VPK: artifacts/iteration55/Touhou75Vita-iteration55.vpk.
WAV de referencia: artifacts/iteration55/th075-effect-002.wav.
SHA256 VPK: f5f61c37bde266830922ca7405d786f8ad221ebfadbbf8b966b4fcd8939e9025.

Devolver los tres logs de iteration55, log.txt y foto; indicar si sonó
al abrir la pantalla y al pulsar cuadrado. Si falta el sonido, el log
permitirá distinguir lectura/decodificación, apertura, volumen y salida.

Referencias primarias del formato y la API:
[arc_unpacker pak1 audio](https://github.com/vn-tools/arc_unpacker/blob/master/src/dec/twilight_frontier/pak1_audio_archive_decoder.cc),
[VitaSDK audioout.h](https://github.com/vitasdk/vita-headers/blob/master/include/psp2/audioout.h).
Se escribió un lector independiente; no se copiaron fuentes del decodificador.
