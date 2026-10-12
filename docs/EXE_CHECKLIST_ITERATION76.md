# Comprobaciones para ejecutar TH075.exe en Vita

Estado tras recibir los logs físicos de la 75: ocho Present y ocho señales/
esperas del temporizador confirmados. La 76 permite hasta dieciséis frames,
optimiza el código nativo y mide el tiempo de sus etapas. Hardware pendiente.

## Orden de trabajo

| # | Comprobación | Criterio concreto | Estado |
|---|---|---|---|
| 1 | Identidad y carga PE32 | SHA del japonés 1.11, secciones/IAT y entrypoint correctos. | Confirmado en Vita. |
| 2 | ABI x86 y memoria | ESP/EIP, registros no volátiles, stdcall, TEB/FS/TLS, heap y accesos válidos. | Confirmado para los servicios usados; faltan casos futuros y ejecución sostenida. |
| 3 | Lectura de recursos original | Rutas, bytes leídos y offsets del DAT coinciden con lo solicitado por el EXE. | Carga inicial/texturas/effects alcanzada; falta cobertura completa. |
| 4 | Workers y sincronización | Threads bloquean/despiertan por eventos/timeout reales y permiten continuar el main. | 75 confirmó ocho SetEvent del worker 12, ocho auto-reset waits consumidos y contextos preservados. 76 extiende a dieciséis; scheduler continuo de todos los workers pendiente. |
| 5 | Texturas y estados D3D8 | Formato, pitch, Lock/Unlock, refs, bindings y viewport válidos. | Confirmados para la ruta inicial. |
| 6 | Primer dibujo del EXE | 307200 cubiertos, 223567 escritos y hash de destino EF895E7C. | Confirmado físicamente en la 72. |
| 7 | Cierre del frame D3DX | EndScene, restore de targets, ApplyStateBlock y Releases sin corrupción. | Confirmado físicamente en la 72. |
| 8 | Segundo quad y fade | ONE/ZERO, diffuse FF0F0F0F, fuente 1024², UV vertical 0..0.46875 y backbuffer X8. | Confirmado físicamente en la 73: 307200 escritos, 219077 cambiados, hash final 957A5E70 y EndScene correcto. |
| 9 | Present original | RECTs/ventana correctos, frame real enviado a scanout, vblank y readback coincidentes. | 75 confirmó frames 1..8, doble buffer alternado, RC 0, matches:yes, limpieza COM y ocho hashes distintos. Parada deliberada antes del noveno. 76 amplía a dieciséis; pendiente en Vita. |
| 10 | Bucle y entrada del menú | Procesar mensajes, reloj, entrada guest y aceptar una selección real. | PeekMessage y ocho vueltas del bucle confirmados en Vita 75. 76 observa contador/estado para superar el fade. Entrada, dispatch y selección real del menú pendientes. |
| 11 | Gráficos completos y memoria sostenida | Geometrías, filtros, mezcla, superficies y destrucción sin fugas; hashes/imágenes comparados con PC. | Renderer rectangular parcial; backend completo pendiente. |
| 12 | Combate, sonido guest y datos persistentes | Entrar a combate, controles, lógica/colisión, mixer, tiempos, round y regreso al menú. | Pendiente. El audio nativo del visor no acreditaba el mixer del EXE. |

Los PASS iniciales acreditan sus contratos. El juego será jugable cuando el
bucle original y las escenas cumplan estos criterios; un contador de imports
o una pantalla de recursos no proporciona un porcentaje de port completado.

## Comprobaciones ejecutadas para la 76

Dieciocho grupos portables con ASan/UBSan: los seis de raster/presentación,
seis de mensajes y seis de waits/contexto descritos en la
[lista anterior](EXE_CHECKLIST_ITERATION75.md).
Se añadió la comprobación de caps compartidos: 16 frames/waits, 45 s antes
del watchdog de 60 s y máximo de 64 slices.

El renderer también se compiló a -O0 y -O2 con -fno-fast-math y
-ffp-contract=off. Los seis grupos aprobaron en ambas configuraciones y la
salida completa coincidió, incluyendo hashes de los patrones propios de
logo, fade y scanout. Los tests conservan sus asserts (sin -DNDEBUG).
Los hashes son una comprobación de regresión sobre esos patrones; no son
una comparación del EXE ni una prueba general de toda geometría/D3D8.

La build Vita usa -O2 -g -DNDEBUG y los flags FP anteriores, comprobados en
flags.make. No hay asserts de runtime en el código propio Vita que sustituyan
sus guardas; esas comprobaciones explícitas permanecen activas.
Compilación y ZIP/SFO/PNG aprobados; ver
[estado y SHA del paquete](STATUS_ITERATION76.md).
No se midió todavía la velocidad de esta build en consola.

## Criterios de la siguiente corrida física

- startup_native_optimization=enabled strict_fp:yes.
- Comparar los primeros ocho scanout hashes con la tabla de la 75.
- Contexto preservado, señal del timer TIB 760000 y wait auto-reset consumido.
- Más de ocho Present, transición del contador original o una nueva frontera
  con stack/EIP. No cambiar el contador ni inyectar una transición.
- Revisar draw_elapsed_us, present_elapsed_us y frame_cycle_elapsed_us.
- Watchdog desarmado y ausencia de fault/corrupción; si se alcanza 16/16,
  el decimoséptimo Present sigue siendo un límite deliberado de la prueba.

## Cómo acelerar las siguientes iteraciones

- Reproducir en PC los datos capturados de cada nueva frontera y comprobar
  píxeles/contratos antes de otra instalación en Vita.
- Preparar varios contratos cuyo ABI/semántica se conocen del binario;
  detenerse en los que siguen sin implementar. No retornar éxito vacío.
- Usar la captura Windows existente como referencia de llamadas/estados;
  capturar otra pasada con apitrace cuando haga falta comparar una escena.
- Análisis estático automático de las diez funciones del arranque/presentación
  listadas en [la investigación](RESEARCH_EXE_ITERATION73.md).
- La build 76 copia al árbol WSL sólo fuentes que cambiaron: conserva timestamps
  y permite compilación incremental. Eliminó includes y ejecución del visor DAT,
  del título nativo independiente y del reproductor BGM de diagnóstico.
- Mantener en Vita los archivos originales: el propio EXE seguirá leyéndolos.

La 76 puede ejecutar y confirmar hasta dieciséis Present reales si ninguna
frontera anterior lo impide. También permite hasta dieciséis continuaciones
de la espera original de frame. Sigue siendo una captura acotada del arranque.
