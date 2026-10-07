# Iteración 56 — DAT Music / 01.60

## Música de comprobación

Al presentar los resultados se elige aleatoriamente una pista de
`ux0:data/TH075Vita/th075bgm.dat`. El archivo original inspeccionado tiene
34 WAV PCM de 44100 Hz, 16 bits, estéreo. No es necesario copiar los WAV
extraídos ni añadirlos al VPK.

| Control | Acción |
| --- | --- |
| Triángulo | Elegir una pista al azar diferente de la actual |
| Cuadrado | Reiniciar la pista actual desde su comienzo |
| X | Detener el audio, esperar al hilo, liberar recursos y salir |

Los botones requieren soltar y volver a pulsar. Se muestra el nombre del
WAV activo. Se quitó la salida automática a los 120 segundos. Al terminar
una pista se repite el WAV completo; todavía no se aplican puntos de loop
ni transiciones musicales del motor original.

## Implementación

- Inventario PAK1 acotado, validación de nombres y límites del archivo.
- Lector RIFF/WAVE que recorre chunks, comprueba formato PCM y limita los
  datos al recurso seleccionado; ignora chunks ajenos sin reescribirlos.
- Streaming desde el DAT con caché PCM de 16 KiB; no carga los 428 MB de
  música en RAM ni la canción completa.
- Interpolación lineal a 48000 Hz estéreo para MAIN, ganancia de puerto 25%.
- Hilo nativo independiente del bucle de pantalla/controles. Órdenes
  atómicas identifican reinicios y cambios; la semilla usa el tiempo de
  proceso y la selección evita repetir inmediatamente al pulsar triángulo.
- Bloques de 1024 frames, conservados hasta su drenaje. Se comprueban
  configuración, volumen, resultados de salida y liberación del puerto.
- Al salir se solicita terminar, se espera al hilo y se borra su handle.

Esto verifica música nativa con los recursos originales. La frontera del
EXE continúa en `IDirectSound8::CreateSoundBuffer`; no implementa todavía
la reproducción del motor mediante DirectSound ni el renderer del menú.

## Evidencia previa y comprobaciones de entrega

El usuario confirmó que escuchó el efecto original de la 55. Su log mostró
nueve reproducciones, con hash PCM coincidente y entrega/drenaje/liberación
correctos. La extracción de las 34 pistas se verificó contra el DAT por
SHA256; cada WAV conserva todos sus bytes originales. La herramienta
`tools/extract_th075_music.py` permite reproducir esa extracción.

La 56 compiló con VitaSDK sin advertencias. CRC ZIP, SFO 01.60/T075VITA1 y
los cuatro PNG indexados de 8 bits sin entrelazado comprobados. Falta la
prueba física de streaming, cambio de pista, reinicio y cierre con X.
No se añadieron ni ejecutaron pruebas sintéticas de implementación.

Build ID: iteration56-dat-music-r1.
VPK local: artifacts/iteration56/Touhou75Vita-iteration56.vpk.
SHA256: 2e6151ac9bb3d3ab123464b84e93b0a1ef83d723cf0e29d9d05a3fdc58e6b3a8.

## Logs para la prueba física

Devolver `iteration56.log`, `iteration56-runtime.log`,
`iteration56-watchdog.log` y el nuevo `iteration56-bgm.log`, además de foto.
El log principal registra inventario, órdenes, pista solicitada y cierre
del hilo. El log BGM registra pista iniciada, formato, progreso, loop,
errores nativos y liberación. Cada hilo escribe en su propio archivo.
Indicar si la música suena sin cortes y si los tres controles responden.
