# Iteración 57 — Continuous Music / 01.61

Resultado físico: el usuario confirmó que la música es fluida. Se
registraron 657 bloques durante 31.058780 segundos, cero preparaciones
tardías y cierre correcto. Cambios/reinicios también aparecen en el log.
La [iteración 58](STATUS_ITERATION58.md) conserva esta ruta y añade visor
de DAT y creación del buffer primario para avanzar el EXE.

## Resultado de la 56

El usuario escuchó música, pero entrecortada. El DAT presentó 34 pistas y
se seleccionó `wave/bgm/63.wav`: PCM estéreo de 16 bits, 44100 Hz y
2157056 frames. Se entregaron 101 bloques sin error de API. El puerto y
el hilo se cerraron correctamente. Estos retornos no prueban continuidad.

La 56 vaciaba la cola mediante `sceAudioOutOutput(port, nullptr)` después
de cada bloque. Preparaba el siguiente solo al terminar el anterior.
Este diseño puede introducir pausas recurrentes, además del coste del
remuestreo. Es la causa probable; falta confirmar la corrección en hardware.

## Compatibilidad WAV y referencias consultadas

WAV es el contenedor: nuestro lector obtiene las muestras PCM. VitaSDK
recibe PCM firmado de 16 bits, mono/estéreo. MAIN requiere 48000 Hz,
mientras BGM admite los 44100 Hz de las pistas originales. No es necesario
convertirlas a MP3 u Ogg para este reproductor.

Se revisó el backend Vita de SDL2: para frecuencias inferiores a 48000 Hz
usa el puerto BGM; alterna dos buffers alineados a 64 bytes y envía bloques
sin drenar la cola entre ellos. La llamada de entrega proporciona espera.
Se adoptó ese patrón mediante código propio, sin incorporar la biblioteca
ni copiar funciones completas.

Fuentes primarias:
[VitaSDK audioout.h](https://raw.githubusercontent.com/vitasdk/vita-headers/master/include/psp2/audioout.h),
[SDL Vita audio](https://raw.githubusercontent.com/libsdl-org/SDL/SDL2/src/audio/vita/SDL_vitaaudio.c),
[SDL Vita buffer count](https://raw.githubusercontent.com/libsdl-org/SDL/SDL2/src/audio/vita/SDL_vitaaudio.h).

## Cambios

- Puerto BGM a 44100 Hz, estéreo; conserva las muestras PCM originales.
- Se elimina el remuestreo a 48000 Hz.
- Dos buffers alternados de 2048 frames, memoria alineada a 64 bytes.
- Se elimina el drenaje por bloque. Los buffers permanecen vivos hasta
  drenar y liberar el puerto al salir, incluso en rutas de error.
- Se eleva un nivel la prioridad del hilo si el valor leído lo permite,
  y se registra el resultado; no se modifican otros hilos.
- Preparación directa del PCM desde la caché de archivo. Al repetir el
  WAV completo se rellena el mismo bloque sin insertar silencio al final.
- Se registran tiempo total, máximo de preparación, máximo de entrega y
  bloques cuya preparación excede su duración. Estos últimos son una
  métrica de demora, no una detección confirmada de underrun.

Triángulo cambia a otra pista aleatoria, cuadrado reinicia la actual y X
cierra la aplicación. No hay salida automática. Se sigue leyendo el DAT
original sin incluir música en el paquete. La reproducción continúa
siendo nativa; el EXE sigue detenido en CreateSoundBuffer.

## Comprobación y prueba física pendiente

Compilación VitaSDK sin advertencias; CRC ZIP, APP_VER 01.61, TITLE_ID
T075VITA1 y cuatro PNG indexados de 8 bits comprobados.
Build ID: iteration57-continuous-music-r1.
VPK: artifacts/iteration57/Touhou75Vita-iteration57.vpk.
SHA256: a71159b2a038c909f017ea91cfafbd8e02ef9f97a9a2cfbeb3d28e8f41b893b2.

Escuchar durante al menos 30 segundos, cambiar y reiniciar canciones, y
salir con X. Devolver iteration57.log, iteration57-bgm.log,
iteration57-runtime.log y iteration57-watchdog.log. Confirmar si persisten
cortes; los tiempos nuevos ayudarán a localizar lectura o entrega tardía.
