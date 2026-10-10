# Comprobaciones para ejecutar TH075.exe en Vita

Hardware 80 creó y cargó el primer buffer de música original, conservó los
203 frames y se detuvo en Play. 81 implementa salida y cursores del PCM
guest; audio audible, refills, nuevos frames y menú todavía no están probados.

## Orden de trabajo

| # | Comprobación | Criterio concreto | Estado |
|---|---|---|---|
| 1 | Identidad y carga PE32 | SHA del japonés 1.11, secciones/IAT y entrypoint correctos. | Confirmado en Vita. |
| 2 | ABI x86 y memoria | ESP/EIP, registros no volátiles, stdcall, TEB/FS/TLS, heap y accesos válidos. | Confirmado para los servicios usados; faltan casos futuros y ejecución sostenida. |
| 3 | Lectura de recursos original | Rutas, bytes leídos y offsets del DAT coinciden con lo solicitado por el EXE. | Carga inicial/texturas/effects alcanzada; falta cobertura completa. |
| 4 | Workers y sincronización | Threads bloquean/despiertan por eventos/timeout reales y permiten continuar el main. | 77 confirmó 203 señales del worker 12, waits auto-reset consumidos y contextos preservados. Scheduler continuo de todos los workers pendiente. |
| 5 | Texturas y estados D3D8 | Formato, pitch, Lock/Unlock, refs, bindings y viewport válidos. | 80 confirmó destrucción del logo y los 24 uploads de opening dentro de 64 MiB. Lifecycle sostenido pendiente. |
| 6 | Primer dibujo del EXE | 307200 cubiertos, 223567 escritos y hash de destino EF895E7C. | Confirmado físicamente en la 72. |
| 7 | Cierre del frame D3DX | EndScene, restore de targets, ApplyStateBlock y Releases sin corrupción. | Confirmado físicamente en la 72. |
| 8 | Segundo quad y fade | ONE/ZERO, diffuse FF0F0F0F, fuente 1024², UV vertical 0..0.46875 y backbuffer X8. | Confirmado físicamente en la 73: 307200 escritos, 219077 cambiados, hash final 957A5E70 y EndScene correcto. |
| 9 | Present original | RECTs/ventana correctos, frame real enviado a scanout, vblank y readback coincidentes. | 77 confirmó 203 frames, RC 0 y matches:yes; primeros dieciséis hashes iguales a 76. 22 preparaciones y 181 reutilizaciones exactas de conversión. |
| 10 | Bucle y entrada del menú | Procesar mensajes, reloj, entrada guest y aceptar una selección real. | 77 alcanzó edad 181, estado 220D, fade de salida y carga siguiente. Entrada y selección real del menú pendientes. |
| 11 | Gráficos completos y memoria sostenida | Geometrías, filtros, mezcla, superficies y destrucción sin fugas; hashes/imágenes comparados con PC. | Renderer rectangular parcial; backend completo pendiente. |
| 12 | Combate, sonido guest y datos persistentes | Entrar a combate, controles, lógica/colisión, mixer, tiempos, round y regreso al menú. | Pendiente. PCM original cargado en 80; mixer/salida nativa de 81 y refill continuo necesitan validación física. |

## Comprobaciones ejecutadas para la 81

54 grupos portables con -Werror, ASan/UBSan: contratos COM/PCM, simulación
de hilo/puerto, fallos nativos, resampling, volumen, looping, cola y lifetime.
Replay de 1312 llamadas D3D y comparación de renderer -O0/-O2 aprobados.
VitaSDK, ZIP/SFO/PNG aprobados. El EXE no se ejecuta en estos tests.
Detalle y límites: [status](STATUS_ITERATION81.md).

## Criterios de la siguiente corrida física

1. Build iteration81-guest-audio-r1, pantalla 81, versión 01.85.
2. Sin regresión de 203 hashes, texturas ni 39 uploads PCM previos.
3. Buffer 40 estéreo de 1 MiB cargado, Play 0/0/1 atendido y ABI correcto.
4. Puerto/hilo/output RC 0; samples no cero enviados; audibilidad confirmada
   por el usuario. Un RC exitoso por sí solo no confirma sonido audible.
5. GetStatus/cursores coherentes, worker 13 despierta por timeout real y rellena
   el buffer mediante sus propias lecturas; medir continuidad, cola y costo.
6. Primer draw/Present de opening o próxima frontera main/worker registrada.
7. Contexto principal preservado; sin CPU fault/limit no explicado; memoria,
   alias/refcounts, drain/join/release y watchdog coherentes.
8. Guardar los tres logs 81, log.txt y foto; X sale del diagnóstico.

## Próximas comprobaciones

Terminar el constructor y dibujo de opening, cubrir sus nuevos perfiles D3D8,
validar la transición e input del menú y luego un combate original. Medir
memoria sostenida, frame timing, cursores/refills, mezclado de efectos y retorno
al menú. La ejecución completa requiere lógica del EXE; los DAT aportan sus
recursos. Referencias y decompiladores: [investigación](RESEARCH_EXE_ITERATION73.md).
Un checkpoint PASS no establece un juego completo ni un porcentaje de avance.
