# Iteración 81 — Guest Audio / 01.85

## Evidencia física de la 80

La foto del celular permite leer la iteración y la frontera Play. Los logs
confirman 203 Present originales y 203 continuaciones con contexto preservado.
Todos los hashes de scanout, uploads de texturas y los 39 uploads PCM previos
coinciden con la 79. Las 24 texturas de opening.dat ya están cargadas; gráficos
vivos: 42896712 bytes. La captura sigue negra después del fade del logo.
Duración: 71025413 us (71,03 s); sin CPU limit, watchdog desarmado.

CreateSoundBuffer aceptó el formato PCM de 16 bytes: canales 2, frecuencia
44100, rate 176400, align 4 y bits 16. El buffer 40, handle 00AE0138, mide
1048576 bytes; total PCM 4151920. QueryInterface Buffer8 y Release conservaron
una referencia. Lock ENTIREBUFFER devolvió 1048576 bytes aunque el argumento
de tamaño era 524288. ReadFile, retorno 408610, cargó exactamente 524288 bytes.
Unlock produjo FNV EE255525 sobre el buffer completo y SetVolume terminó.
Play en BF40C0 fue la frontera, con ESP 9FEBE8 / EBP 9FEC08.

La ruta estática original 407D4C..407D5B pasa reserved1=0, reserved2=0,
flags=1 (LOOPING) y retorna a 407D5E. La 80 aún no leyó/atendió esa llamada.
Crear un buffer y registrar su hash no acredita reproducción.

## Cambios de la 81

1. Play inicia salida nativa del PCM que el EXE cargó mediante Lock/Unlock.
   No selecciona ni abre música por su cuenta. Se conserva la lógica original.
2. Hilo nativo con stack 64 KiB, bloques de 1024 frames, dos buffers alineados
   a 64 bytes, salida S16 estéreo 48 kHz y conversión lineal desde el formato
   original. Mezcla saturada, volumen logarítmico y pan por voz; hasta 128
   handles existentes, 16 MiB de PCM total y 4 MiB por buffer.
3. Looping, GetStatus, Stop, seek y GetCurrentPosition. La fase usa enteros;
   el cursor descuenta los frames que devuelve sceAudioOutGetRestSample.
   Es una estimación por bloques, no lectura del reloj de muestras físico.
   El cursor de escritura protege audio en cola, un bloque preparado pendiente
   de aceptación y el lookahead del filtro; su margen es conservador.
4. La fase se confirma después de un Output exitoso. Seek/Stop/Release
   invalidan commits antiguos mediante epochs. Un mutex protege los samples
   y estados; el Output que bloquea se ejecuta fuera del mutex.
5. El worker original 13 puede atender COM DirectSound al vencer realmente
   su timeout de 80 ms en un wait observado del main. Se preservan contexto,
   TIB, FPU, stack y last-error. Límite de 4096 bloques y 32 servicios por wake.
   Una llamada no cubierta o un límite real detiene el diagnóstico.
6. Se registran blocks, muestras no cero, primer hash/pico, tiempo máximo de
   preparación y errores. Al parar se drena, espera y elimina el hilo antes
   de liberar el puerto. Los errores de cola/output se propagan al resultado.

Los samples ya enviados pueden sonar hasta que se vacíe la cola tras Stop o
Release. La resolución/latencia de cursores, la carga del hilo y la reposición
continua necesitan medición física. Solo se cargó media región: 524288 /
176400 ≈ 2,97 s. Si la lógica original no llega a rellenarla, el resto sigue
en silencio. No se inventan lecturas ni se rellena con otra pista.

## Contratos y recursos consultados

Play parte del cursor actual; repetirlo con los mismos flags conserva el avance.
Cambiar el modo looping durante reproducción permanece como frontera explícita.
Se atienden
reservados cero y flags 0/1. Otros flags siguen como frontera explícita.
[Microsoft: Play](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/mt708933(v=vs.85)).
Stop conserva posición para la siguiente reproducción.
[Microsoft: Stop](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/mt708940(v=vs.85)).
Los cursores DirectSound son offsets de bytes y admiten outputs opcionales.
[Microsoft: GetCurrentPosition](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/mt708925(v=vs.85)).
La salida Vita recibe PCM S16 intercalado; Output bloquea y nullptr drena,
y GetRestSample devuelve frames pendientes. El puerto MAIN usa 48 kHz.
[VitaSDK: SceAudio](https://docs.vitasdk.org/group__SceAudioUser.html).

Se mantienen WinVita/Box86, el lector local TH075 y las referencias ya
documentadas de N0zoM1z0, VitaSDK y otros ports. No se incorporó código de
Wine ni del juego descompilado al repositorio; el nuevo mixer es propio.
Véase [investigación previa](RESEARCH_EXE_ITERATION73.md).

## Comprobaciones y paquete

54 grupos portables a -O2, -Werror, ASan/UBSan: 34 regresiones previas, 13
grupos del puente PCM y siete del mixer. Memoria y muestras sintéticas;
APIs nativas sustituidas por una simulación de hilo/puerto. No se ejecutan
el EXE, DAT, ARM/VitaSDK ni la consola en estos tests.

- Formato PCM de 16 bytes, arena límite, tamaños/formats inválidos, silencio,
  identidad Buffer8, medio upload, Lock circular y GetFormat.
- Play y samples enviados a la API simulada, status, Play repetido, seek,
  Stop, Release mientras hay Output pendiente y cierre de referencias.
- Fallos de volumen, CreateThread, StartThread, Output y consulta de cola;
  el servicio no consume el frame cuando falla su contrato.
- Mono 8-bit, estéreo 16-bit, conversión 44100→48000, fase exacta de ocho
  segundos sin deriva, wrap, cola, EOF, epochs, clipping, volumen/pan y frecuencia.

Replay de 1312 llamadas D3D observadas en hardware, sin ejecutar x86;
renderer -O0/-O2 con outputs/digests idénticos. El dispatch de instrucciones
del worker original todavía no se comprobó en PC ni en hardware 81.

VitaSDK compiló con -O2, símbolos y FP estricto. ZIP CRC, SFO
01.85/T075VITA1 y cuatro PNG indexados de 8 bits sin entrelazado aprobados.
Build: iteration81-guest-audio-r1.
VPK local: artifacts/iteration81/Touhou75Vita-iteration81.vpk.
SHA256: 1b2bac426bf44219cdbc75f780a1b67df40dba51d0e946dce7fe640265338362.
LiveArea actualizado con image_gen integrado, badge 81/01.85. Prompt y rutas
guardados en artifacts/iteration81/livearea-generation.txt, fuera de Git.

## Próxima corrida en Vita

1. Instalar la 81/01.85 y conservar EXE/DAT originales.
2. Esperar hasta 100 s al diagnóstico (watchdog 120 s). X sale.
3. Enviar iteration81.log, iteration81-runtime.log, iteration81-watchdog.log,
   log.txt y foto/captura. Indicar si sonó música y si fue continua o se cortó.
4. Confirmar la creación/carga previa, Play y su ABI (cleanup 20, retorno
   407D5E); revisar output_pipeline/summary, blocks, nonzero_samples y RC.
5. Revisar worker_resumed_com, startup_audio_worker_dispatches, cursores y
   nuevos Lock/ReadFile/Unlock. Registrar su próxima frontera sin omitirla.
6. Verificar dibujos/Present posteriores a 203, escena/estado y presupuesto
   de memoria. No se garantiza alcanzar 240 ni entrar al menú en esta versión.

El VPK 81 está generado; su corrida física está pendiente. El audio fluido
confirmado antes correspondía al reproductor diagnóstico retirado en 73.
Abrir y jugar el menú/combate original sigue pendiente.
Véanse [checklist](EXE_CHECKLIST_ITERATION81.md) y [roadmap](ROADMAP.md).
