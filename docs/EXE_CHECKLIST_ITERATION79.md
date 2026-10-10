# Comprobaciones para ejecutar TH075.exe en Vita

La 78 confirmó que el logo se destruye y recupera 2 MiB. Cargó tres texturas
adicionales y volvió a topar con el cap de 32 MiB: dieciocho imágenes de opening
cargadas y la decimonovena rechazada. La 79 amplía a 64 MiB y observa memoria.
Hardware 79, píxeles de opening, menú y combate interactivos pendientes.

## Orden de trabajo

| # | Comprobación | Criterio concreto | Estado |
|---|---|---|---|
| 1 | Identidad y carga PE32 | SHA del japonés 1.11, secciones/IAT y entrypoint correctos. | Confirmado en Vita. |
| 2 | ABI x86 y memoria | ESP/EIP, registros no volátiles, stdcall, TEB/FS/TLS, heap y accesos válidos. | Confirmado para los servicios usados; faltan casos futuros y ejecución sostenida. |
| 3 | Lectura de recursos original | Rutas, bytes leídos y offsets del DAT coinciden con lo solicitado por el EXE. | Carga inicial/texturas/effects alcanzada; falta cobertura completa. |
| 4 | Workers y sincronización | Threads bloquean/despiertan por eventos/timeout reales y permiten continuar el main. | 77 confirmó 203 señales del worker 12, waits auto-reset consumidos y contextos preservados. Scheduler continuo de todos los workers pendiente. |
| 5 | Texturas y estados D3D8 | Formato, pitch, Lock/Unlock, refs, bindings y viewport válidos. | 78 confirmó destrucción/refcount del logo; el cap de 32 MiB queda corto para opening. 79 permite 64 MiB y observa consumo real. |
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

## Comprobaciones ejecutadas para la 79

34 grupos propios a -O2, -Werror y ASan/UBSan: raster/conversión y límites,
mensajes, waits/contexto, caché/miniatura, almacenamiento/refcounts D3D8 y
strings guest. Casos concretos en [la lista de la 78](EXE_CHECKLIST_ITERATION78.md).
El caso de cap/rechazo/liberación se ejecuta ahora contra el presupuesto de
64 MiB y conserva el fallo explícito antes de recuperar memoria.

Además:

- Reproducción de 1295 llamadas de propiedad/estado registradas en la 78;
  el logo se libera y su última CreateTexture tiene éxito.
- Comparación de dieciocho tamaños/formato físicos con headers de opening.dat.
  Coinciden con redondeo a potencia de dos y formato 21/A8R8G8B8.
- Proyección de las cinco asignaciones posteriores al último call reproducido,
  terminando en 42896712 bytes. Presupuesto y GetAvailableTextureMem coherentes.
- Renderer a -O0/-O2 con salidas/digests idénticos.
- ELF del SDK anterior/nuevo: default heap 134217728 bytes; nm confirma el
  enlace de _get_vita_heap_size, mallinfo y sceKernelGetFreeMemorySize.
- VPK compilado con FP estricto; ZIP CRC, SFO 01.83/T075VITA1 y cuatro PNG
  indexados de ocho bits sin entrelazado correctos.

Los tests portables no ejecutan el EXE ni uploads/game pixels/SDK. La proyección
comprueba tamaños, no un flujo guest completado. Las observaciones nativas
recién añadidas están compiladas/enlazadas y deben medirse en Vita. No se inyectó
un fallo real de heap, sysmem o display. Los artefactos/trazas quedan fuera de Git.

## Criterios de la siguiente corrida física

1. Build iteration79-texture-budget-r1, pantalla 79 y versión 01.83.
2. Inicialmente startup_native_heap reserved:134217728; observar managed,
   in_use, free_chunks y top_free separadamente de startup_kernel_memory.
   Un query fallido muestra values:unavailable, sin inferir cero memoria libre.
3. Misma salida del logo y destrucción ABC938/2097152 bytes. Asignación antes
   rechazada de 512x512 aceptada; comprobar las seis imágenes aún no cargadas.
4. El storage peak debe superar el antiguo cap. Si solo continúa la carga
   identificada, el tamaño proyectado es 42896712 bytes; otras asignaciones
   nuevas se deben analizar según sus argumentos reales.
5. Capturar la primera escena/vtable, draw/estado y Present después del 203,
   o la frontera exacta que impidió alcanzarlos.
6. startup_d3d8_storage_summary registra used/peak/cap y handles. Distinguir
   storage_budget, native_heap y handle_capacity en cualquier nuevo rechazo.
   Si vuelve MessageBoxA, leer el texto/caption CP932 del log.
7. Watchdog desarmado y contexto válido. Los caps de trabajo/tiempo siguen
   explícitos; llegar a 240 frames no acredita un menú jugable.

## Próximas comprobaciones

Terminar carga y dibujo de opening, ampliar únicamente los perfiles D3D8 que
el EXE solicite, verificar transición/entrada del menú y scheduler continuo.
Para memoria sostenida, medir picos/fragmentación y liberar según ownership;
si los recursos vivos exceden la memoria disponible, estudiar almacenamiento
reversible, streaming o formatos compatibles sin modificar sus píxeles.
Los DAT no sustituyen la lógica ejecutable. Los proyectos/decompiladores
siguen documentados en [la investigación](RESEARCH_EXE_ITERATION73.md).
