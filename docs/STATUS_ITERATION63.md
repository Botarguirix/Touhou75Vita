# Iteración 63 — PCM Buffers / 01.67

## Evidencia física de la 62

El segundo worker original 00407EF0 creó su evento AB2008, guardó una
espera de 80 ms, obtuvo handle AB4010/ID 13 y se reanudó tras 162523 us.
Retornó a su espera real sin fallo ni límite de CPU. El principal llegó
a CreateSoundBuffer: descriptor 36, flags 00058080, bytes 32608,
formato guest 009FEBE8, output 009FEB88, agregación cero. El formato PCM
no se registró en la 62; se inspeccionará y validará durante la 63.
Se mantuvieron 61 llamadas D3D, seis DInput y cuatro DirectSound;
dos threads creados y tiempo de arranque 7.991918 segundos.

## Buffers secundarios

CreateSoundBuffer atiende la familia observada de flags 00058080 cuando
el descriptor corresponde a PCM válido. WAVEFORMATEX: tag 1, mono/stereo,
8/16 bits, 11025..48000 Hz, block align y byte rate exactos, cbSize cero.
GUID/atributos reservados/agregación deben ser cero. Otros formatos y
flags conservan una frontera explícita. No se presupone que el formato
del juego sea uno concreto: se registra y se valida antes de crear.

Máximo 128 buffers, cuatro MiB por buffer, 16 MiB total de almacenamiento
nativo. Handles AE0000.., vtable AE1000, traps BF4000. Cada buffer mantiene
referencias y un pin del objeto padre, formato, controles y muestras;
el último Release libera almacenamiento y anula el handle. Se inicializa
silencio PCM (128 para 8 bits, cero para 16 bits).

Lock/Unlock usan staging guest 01800000..01C00000, separado del staging
gráfico. Se comprueba su pertenencia a la arena. Lock devuelve uno o dos
segmentos reales, comprueba alineación, tamaño y outputs, admite flags
cero/ENTIREBUFFER y conserva el contenido. Solo se permite un lock
simultáneo. Unlock valida los cuatro valores devueltos y recupera PCM,
registrando FNV1a. No se acepta Unlock de un buffer diferente.

Se implementan IUnknown/IDirectSoundBuffer QI, AddRef/Release, posición
inicial, volumen/pan de almacenamiento y GetStatus detenido (ningún Play
ha sido atendido). Play, Stop, mezclado y GetCurrentPosition de reproducción
siguen siendo fronteras. Estos contratos no anuncian sonido reproducido
por el EXE. La música DAT del visor sigue por su ruta nativa independiente.

## Siguiente captura

Build iteration63-pcm-buffers-r1, versión 01.67, TITLE_ID T075VITA1.
VPK artifacts/iteration63/Touhou75Vita-iteration63.vpk.
Controles de música/visor conservados. Devolver iteration63.log,
iteration63-runtime.log, iteration63-watchdog.log, iteration63-bgm.log,
log.txt y foto. Revisar startup_dsound_pcm_format, secondary,
pcm_lock/pcm_upload y nueva startup_stop_import. El próximo paso dependerá
del contrato observado; título original y gameplay siguen pendientes.

VitaSDK compiló sin advertencias. CRC ZIP, SFO 01.67 y cuatro PNG
indexados de 8 bits sin entrelazado inspeccionados.
SHA256: a889bb4c9364675373b6263ec1ba21de7d2edc5531bcd4c6f5341a7812b58a3f.
Validación de formatos y cargas PCM pendiente en hardware.

