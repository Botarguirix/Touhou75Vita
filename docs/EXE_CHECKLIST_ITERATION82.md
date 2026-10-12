# Comprobaciones para ejecutar TH075.exe en Vita

Hardware 81 confirmó sonido original breve y una reposición desde su worker.
Los 203 frames previos coinciden con 80. El siguiente DrawPrimitiveUP requiere
LINEAR; 82 lo implementa para el perfil rectangular observado. Menú pendiente.

## Orden de trabajo

| # | Comprobación | Criterio concreto | Estado |
|---|---|---|---|
| 1 | Identidad y carga PE32 | SHA del japonés 1.11, secciones/IAT y entrypoint correctos. | Confirmado en Vita. |
| 2 | ABI x86 y memoria | ESP/EIP, registros no volátiles, stdcall, TEB/FS/TLS, heap y accesos válidos. | Confirmado para los servicios usados; faltan casos futuros y ejecución sostenida. |
| 3 | Lectura de recursos original | Rutas, bytes leídos y offsets del DAT coinciden con lo solicitado por el EXE. | Carga inicial/texturas/effects alcanzada; falta cobertura completa. |
| 4 | Workers y sincronización | Threads bloquean/despiertan por eventos/timeout reales y permiten continuar el main. | 77 confirmó 203 señales del worker 12, waits auto-reset consumidos y contextos preservados. 81 confirmó un timeout y ocho servicios del worker de audio 13. Scheduler continuo de todos los workers pendiente. |
| 5 | Texturas y estados D3D8 | Formato, pitch, Lock/Unlock, refs, bindings y viewport válidos. | 81 confirmó destrucción del logo y los 24 uploads de opening dentro de 64 MiB. Lifecycle sostenido pendiente. |
| 6 | Primer dibujo del EXE | 307200 cubiertos, 223567 escritos y hash de destino EF895E7C. | Confirmado físicamente en la 72. |
| 7 | Cierre del frame D3DX | EndScene, restore de targets, ApplyStateBlock y Releases sin corrupción. | Confirmado físicamente en la 72. |
| 8 | Segundo quad y fade | ONE/ZERO, diffuse FF0F0F0F, fuente 1024², UV vertical 0..0.46875 y backbuffer X8. | Confirmado físicamente en la 73: 307200 escritos, 219077 cambiados, hash final 957A5E70 y EndScene correcto. |
| 9 | Present original | RECTs/ventana correctos, frame real enviado a scanout, vblank y readback coincidentes. | 77 confirmó 203 frames, RC 0 y matches:yes; primeros dieciséis hashes iguales a 76. 22 preparaciones y 181 reutilizaciones exactas de conversión. |
| 10 | Bucle y entrada del menú | Procesar mensajes, reloj, entrada guest y aceptar una selección real. | 77 alcanzó edad 181, estado 220D, fade de salida y carga siguiente. Entrada y selección real del menú pendientes. |
| 11 | Gráficos completos y memoria sostenida | Geometrías, filtros, mezcla, superficies y destrucción sin fugas; hashes/imágenes comparados con PC. | POINT rectangular confirmado; LINEAR implementado en 82, pendiente en Vita. Backend completo y comparación Windows pendientes. |
| 12 | Combate, sonido guest y datos persistentes | Entrar a combate, controles, lógica/colisión, mixer, tiempos, round y regreso al menú. | 81 confirmó sonido breve y una reposición original. Continuidad, combate, controles y persistencia pendientes. |

## Comprobaciones ejecutadas para la 82

64 grupos portables con -Werror y ASan/UBSan; POINT/LINEAR -O0/-O2 idénticos;
replay de 1312 llamadas D3D; VitaSDK y ZIP/SFO/PNG aprobados.
Detalle, referencias y límites: [status](STATUS_ITERATION82.md).

## Criterios de la siguiente corrida física

1. Build iteration82-linear-quads-r1, pantalla 82, versión 01.86.
2. Primeros 203 hashes y uploads previos sin regresión.
3. Draw 231 aceptado con LINEAR 2/2/2, textura 512² y un nivel; revisar
   source/destination hashes, alpha, clip y siguiente perfil original.
4. Present posterior a 203 con frame real y readback coincidente.
5. PCM original sin errores de output; audibilidad confirmada por el usuario,
   duración/continuidad medidas. No confundir RC 0 con sonido audible.
6. Worker 13 rellena mediante sus lecturas, mantiene su contexto y bloquea
   de nuevo; cursores avanzan salvo seek/Stop o wrap legítimo del buffer.
7. Memoria/refcounts, límites, contexto main, drain/join/release y watchdog
   coherentes. Guardar tres logs 82, log.txt y foto; X sale.

## Próximas comprobaciones

Completar el dibujo/transición de opening, cubrir los perfiles originales
que aparezcan, comparar con Windows y llegar a una selección real del menú.
Después, controles/colisiones/round, audio sostenido, tiempos, memoria y regreso
al menú. Los DAT son recursos; la lógica completa sigue requiriendo el EXE.
Referencias/decompiladores: [investigación](RESEARCH_EXE_ITERATION73.md).
Un checkpoint PASS no establece un juego completo ni un porcentaje de avance.
