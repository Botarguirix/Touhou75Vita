# Iteración 80 — PCM Format / 01.84

## Evidencia física de la 79

La foto tomada con celular permite leer la versión y la frontera DirectSound.
Los logs son la evidencia precisa: 203 Present originales, 203 señales reales
del timer y 203 continuaciones con contexto preservado. Todos los hashes de
scanout coinciden con la 78; set/vblank/query RC 0 y matches:yes en cada frame.
Logo edad 181, estado 220D y fade negro completo. 22 conversiones preparadas,
181 reutilizaciones; mediana de Present 37788 us. La corrida tardó 70308376 us
(70,31 s). Sin CPU limit, watchdog desarmado. Las duraciones entre corridas no
constituyen un benchmark controlado.

La destrucción del logo recuperó 2097152 bytes y dejó 27413320 usados.
Después se asignaron y cargaron mediante Lock/Unlock las 24 imágenes de
opening.dat. Dimensiones, formato y orden coinciden con sus headers locales;
cada objeto tiene su registro de upload/hash. Los gráficos vivos llegaron
exactamente a 42896712 bytes (40,91 MiB), la proyección de la 79. Cero rechazos
de asignación. Esto acredita la carga de texturas; el constructor de la escena
aún no terminó y no hay un nuevo frame de opening ni menú jugable.

Los 39 buffers PCM anteriores conservaron sus 3103344 bytes y todos los hashes
de upload de la 78. Hubo 5855 servicios D3D8, 187 DInput, 199 DirectSound,
14313 imports main y 31 slices. log.txt conserva inicializaciones y LoadEffect OK.

## Memoria medida en Vita

Los nuevos observadores ejecutaron consultas reales. Al detenerse:

| Contador | Bytes |
|---|---:|
| Heap reservado de newlib | 134217728 |
| Heap administrado | 57831424 |
| Heap en uso | 57415616 |
| Chunks libres / chunk superior | 415808 / 170160 |
| Storage gráfico vivo y pico | 42896712 |
| Límite gráfico | 67108864 |
| Kernel libre USER / CDRAM / PHYCONT | 62914560 / 113246208 / 18874368 |

Storage registra 175 recursos propietarios, 170 aliases vivos y 347 handles
históricamente asignados de 512. El heap ya reservado y el USER libre del
kernel son dominios distintos. Chunks libres no incluyen todo el espacio aún
sin administrar de la reserva ni garantizan una asignación contigua. La 80
conserva estos observadores y límites; no aumenta el heap nativo.

## Causa exacta del siguiente bloqueo

CreateSoundBuffer: descriptor 36 bytes, flags 00058080, buffer 1048576 bytes,
reserved y GUID 3D cero, sin agregación. Descriptor 009FEA94, formato 009FEAFC,
output 009FEA90. PCM tag 1, dos canales, 44100 Hz, rate 176400, align 4,
bits 16. EIP BF6030, ESP 9FEA68, EBP 9FEABC. Método no ejecutado.

El puente leía 18 bytes y exigía cbSize cero. El último word leído era 24932,
0x6164, bytes ASCII 64 61 / da. El EXE original en 408455 lee 18 bytes desde
el WAV al formato en su stack; 406A98 lo entrega a CreateSoundBuffer.
La auditoría local de las 34 pistas de th075bgm.dat confirma RIFF/WAVE,
fmt de 16 bytes, PCM 1/2/44100/176400/4/16 y chunk data inmediatamente después.
Por eso los dos bytes sobrantes son el inicio de data, no un formato comprimido.
No se modifican el EXE, DAT ni el lector original.

Microsoft especifica que cbSize se ignora para WAVE_FORMAT_PCM.
[Contrato WAVEFORMATEX](https://learn.microsoft.com/en-us/windows/win32/api/mmeapi/ns-mmeapi-waveformatex).
La 80 lee solo los 16 bytes necesarios y registra format_bytes_read:16,
cbsize:ignored_pcm. Conserva tag 1, mono/estéreo, 8/16 bits, 11025..48000 Hz,
rate/align exactos, flags observados, tamaño alineado, capacidad individual
4 MiB, total PCM 16 MiB y 128 handles. Formatos comprimidos siguen sin atenderse.
Los GetFormat existentes devuelven la representación canónica de 18 bytes
con cbSize cero. No se añade un resultado de Play ni se mueve un cursor detenido.

## Comprobaciones para la 80

La nueva regresión falló antes de la corrección en CreateSoundBuffer y pasó
después. Siete grupos PCM nuevos junto a los 34 portables anteriores:

1. Descriptor de la 79, 1 MiB, estéreo y bytes da después del prefix PCM.
2. Formato de 16 bytes al final del arena, sin leer fuera de él.
3. Tags/channels/rate/align/bits inválidos, tamaños desalineados y excesivos.
4. Silencio de 8/16 bits, lock y Release de último ref bloqueado/rechazado.
5. Identidad Buffer8, upload sintético de medio MiB y Lock circular de dos spans.
6. GetFormat canónico, status detenido y frontera Play explícita.
7. Referencias del puerto simulado equilibradas sin enviar audio.

41 grupos a -O2, -Werror y ASan/UBSan aprobados. Replay de 1312 llamadas reales
D3D8 de la 79, logo liberado y asignaciones posteriores aceptadas. Renderer
a -O0/-O2 con outputs/digests iguales. Los tests usan memoria propia y muestras
sintéticas; los puertos se simulan. No ejecutan x86, SDK, audio, DAT ni draws
del juego. Los logs de Vita y headers de recursos se auditan por separado.

VitaSDK compiló el VPK con -O2, símbolos y FP estricto. ZIP CRC, SFO
01.84/T075VITA1 y cuatro PNG indexados de ocho bits sin entrelazado aprobados.
Build: iteration80-pcm-format-r1.
VPK local: artifacts/iteration80/Touhou75Vita-iteration80.vpk.
SHA256: 8ca1f4d2ac39321de038c0a406519f553f087b2a4887e08bb178be3131b9ff19.
LiveArea actualizado con image_gen integrado, solo badge 80/01.84; prompt,
imagen de entrada y salida guardados fuera de Git en livearea-generation.txt.

## Próxima corrida en Vita

1. Instalar la 80/01.84 y conservar el EXE/DAT originales.
2. Esperar hasta 100 s al diagnóstico, watchdog 120 s. X sale.
3. Enviar iteration80.log, iteration80-runtime.log, iteration80-watchdog.log,
   log.txt y foto/captura legible de STOP/PRESENT FRAMES. La foto de celular sirve.
4. Confirmar las 24 texturas, los 39 uploads PCM anteriores y el formato estéreo
   aceptado con 16 bytes leídos. El siguiente buffer deberá ser el número 40,
   1048576 bytes y total PCM 4151920 si no hay nuevas liberaciones/asignaciones.
5. Comprobar QI Buffer8 y las llamadas posteriores de Lock/ReadFile/Unlock
   solicitadas por el EXE, o su próxima frontera real. Observar cualquier
   primer frame posterior al 203 y la nueva escena con sus píxeles originales.

La creación original que sigue puede usar medio buffer para streaming
(ruta estática 4085A8..408656). Esa ruta aún no se ejecutó en Vita; las muestras
sintéticas solo comprueban almacenamiento y ABI. Play, avance de cursores,
resampling/mezclador guest, audio worker continuo e interacción siguen pendientes.
No hay música/visor de sprites de diagnóstico. Se mantienen 240 frames/waits,
100 s, 512 draws, 128 Mi píxeles y 64 slices.
Véanse [checklist](EXE_CHECKLIST_ITERATION80.md), [roadmap](ROADMAP.md) y
[recursos/decompiladores revisados](RESEARCH_EXE_ITERATION73.md).
