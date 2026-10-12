# Comprobaciones para ejecutar TH075.exe en Vita

La 79 confirmó todas las 24 texturas y uploads de opening.dat, 42896712 bytes
de gráficos vivos y 203 Present reales. El siguiente buffer PCM estéreo fue
rechazado por leer cbSize desde el chunk data posterior a su formato de 16 bytes.
La 80 corrige esa lectura. Constructor, nuevos frames, audio y menú pendientes.

## Orden de trabajo

| # | Comprobación | Criterio concreto | Estado |
|---|---|---|---|
| 1 | Identidad y carga PE32 | SHA del japonés 1.11, secciones/IAT y entrypoint correctos. | Confirmado en Vita. |
| 2 | ABI x86 y memoria | ESP/EIP, registros no volátiles, stdcall, TEB/FS/TLS, heap y accesos válidos. | Confirmado para los servicios usados; faltan casos futuros y ejecución sostenida. |
| 3 | Lectura de recursos original | Rutas, bytes leídos y offsets del DAT coinciden con lo solicitado por el EXE. | Carga inicial/texturas/effects alcanzada; falta cobertura completa. |
| 4 | Workers y sincronización | Threads bloquean/despiertan por eventos/timeout reales y permiten continuar el main. | 77 confirmó 203 señales del worker 12, waits auto-reset consumidos y contextos preservados. Scheduler continuo de todos los workers pendiente. |
| 5 | Texturas y estados D3D8 | Formato, pitch, Lock/Unlock, refs, bindings y viewport válidos. | 79 confirmó destrucción del logo y los 24 uploads de opening dentro de 64 MiB. Lifecycle sostenido pendiente. |
| 6 | Primer dibujo del EXE | 307200 cubiertos, 223567 escritos y hash de destino EF895E7C. | Confirmado físicamente en la 72. |
| 7 | Cierre del frame D3DX | EndScene, restore de targets, ApplyStateBlock y Releases sin corrupción. | Confirmado físicamente en la 72. |
| 8 | Segundo quad y fade | ONE/ZERO, diffuse FF0F0F0F, fuente 1024², UV vertical 0..0.46875 y backbuffer X8. | Confirmado físicamente en la 73: 307200 escritos, 219077 cambiados, hash final 957A5E70 y EndScene correcto. |
| 9 | Present original | RECTs/ventana correctos, frame real enviado a scanout, vblank y readback coincidentes. | 77 confirmó 203 frames, RC 0 y matches:yes; primeros dieciséis hashes iguales a 76. 22 preparaciones y 181 reutilizaciones exactas de conversión. |
| 10 | Bucle y entrada del menú | Procesar mensajes, reloj, entrada guest y aceptar una selección real. | 77 alcanzó edad 181, estado 220D, fade de salida y carga siguiente. Entrada y selección real del menú pendientes. |
| 11 | Gráficos completos y memoria sostenida | Geometrías, filtros, mezcla, superficies y destrucción sin fugas; hashes/imágenes comparados con PC. | Renderer rectangular parcial; backend completo pendiente. |
| 12 | Combate, sonido guest y datos persistentes | Entrar a combate, controles, lógica/colisión, mixer, tiempos, round y regreso al menú. | Pendiente. El audio nativo del visor no acreditaba el mixer del EXE. |

Los PASS iniciales acreditan sus contratos. El juego será jugable cuando el
bucle original y las escenas cumplan estos criterios; un contador de imports
o una pantalla de recursos no proporciona un porcentaje de port completado.

## Comprobaciones ejecutadas para la 80

41 grupos portables a -O2, -Werror y ASan/UBSan; los 34 anteriores y siete
grupos nuevos sobre la clase DirectSoundBootstrap que se compila para Vita.
Regresión comprobada antes/después del fix; prefix PCM de 16 bytes, límite
de memoria, formatos/tamaños inválidos, silencio 8/16, COM Buffer8, references,
medio upload sintético, Lock circular, GetFormat, status y Play no atendido.
Puerto nativo simulado sin muestras enviadas. Detalle en [status](STATUS_ITERATION80.md).

Replay de 1312 calls D3D de hardware 79: propiedad y asignaciones dentro del
cap. Renderer -O0/-O2 con outputs/digests idénticos. Auditoría independiente
de 24 uploads físicos y 34 WAVs locales con fmt de 16 bytes + data.
VitaSDK y ZIP/SFO/PNG aprobados. Los tests no ejecutan EXE, SDK ni audio.

## Criterios de la siguiente corrida física

1. Build iteration80-pcm-format-r1, pantalla 80, versión 01.84.
2. 203 frames previos, misma salida del logo, 24 texturas cargadas; no regresión
   en los 39 buffers PCM/3103344 bytes y sus hashes anteriores.
3. CreateSoundBuffer estéreo 1 MiB aceptado, format_bytes_read:16 y cbsize:ignored_pcm.
4. Buffer 40, total PCM 4151920 bytes si no intervienen otros recursos; observar
   QI/Lock/ReadFile/Unlock posteriores con contextos y ABI correctos.
5. Primera nueva escena/draw/Present o servicio no atendido registrado fielmente.
   Si llega Play, conservarlo como frontera hasta implementar reproducción real.
6. Heap/storage usados, pico y aliases/handles coherentes; cualquier rechazo
   identificado como cap, heap o contrato. Contextos válidos, watchdog desarmado.
7. Foto de celular o captura y los cuatro logs; llegar a 240 frames no acredita
   un menú ni un combate jugable.

## Próximas comprobaciones

Terminar carga y dibujo de opening, ampliar únicamente los perfiles D3D8 que
el EXE solicite, verificar transición/entrada del menú y scheduler continuo.
Para memoria sostenida, medir picos/fragmentación y liberar según ownership;
si los recursos vivos exceden la memoria disponible, estudiar almacenamiento
reversible, streaming o formatos compatibles sin modificar sus píxeles.
Los DAT no sustituyen la lógica ejecutable. Los proyectos/decompiladores
siguen documentados en [la investigación](RESEARCH_EXE_ITERATION73.md).
