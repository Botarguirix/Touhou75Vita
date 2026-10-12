# Comprobaciones para ejecutar TH075.exe en Vita

Estado tras recibir los logs físicos de la 73. La 74 atiende PeekMessageA;
su integración y el Present nativo todavía necesitan ejecución en consola.

## Orden de trabajo

| # | Comprobación | Criterio concreto | Estado |
|---|---|---|---|
| 1 | Identidad y carga PE32 | SHA del japonés 1.11, secciones/IAT y entrypoint correctos. | Confirmado en Vita. |
| 2 | ABI x86 y memoria | ESP/EIP, registros no volátiles, stdcall, TEB/FS/TLS, heap y accesos válidos. | Confirmado para los servicios usados; faltan casos futuros y ejecución sostenida. |
| 3 | Lectura de recursos original | Rutas, bytes leídos y offsets del DAT coinciden con lo solicitado por el EXE. | Carga inicial/texturas/effects alcanzada; falta cobertura completa. |
| 4 | Workers y sincronización | Threads bloquean/despiertan por eventos/timeout reales y permiten continuar el main. | Handoff y wakes parciales confirmados; scheduler continuo pendiente. |
| 5 | Texturas y estados D3D8 | Formato, pitch, Lock/Unlock, refs, bindings y viewport válidos. | Confirmados para la ruta inicial. |
| 6 | Primer dibujo del EXE | 307200 cubiertos, 223567 escritos y hash de destino EF895E7C. | Confirmado físicamente en la 72. |
| 7 | Cierre del frame D3DX | EndScene, restore de targets, ApplyStateBlock y Releases sin corrupción. | Confirmado físicamente en la 72. |
| 8 | Segundo quad y fade | ONE/ZERO, diffuse FF0F0F0F, fuente 1024², UV vertical 0..0.46875 y backbuffer X8. | Confirmado físicamente en la 73: 307200 escritos, 219077 cambiados, hash final 957A5E70 y EndScene correcto. |
| 9 | Present original | RECTs/ventana correctos, frame real enviado a scanout, vblank y readback coincidentes. | Implementado 73 con límite de ocho frames; llamada nativa/ABI en Vita pendientes. |
| 10 | Bucle y entrada del menú | Procesar mensajes, reloj, entrada guest y aceptar una selección real. | 73 llegó a PeekMessageA; 74 atiende la consulta sobre la cola propia. Entrada por mensajes, dispatch y selección del menú pendientes. |
| 11 | Gráficos completos y memoria sostenida | Geometrías, filtros, mezcla, superficies y destrucción sin fugas; hashes/imágenes comparados con PC. | Renderer rectangular parcial; backend completo pendiente. |
| 12 | Combate, sonido guest y datos persistentes | Entrar a combate, controles, lógica/colisión, mixer, tiempos, round y regreso al menú. | Pendiente. El audio nativo del visor no acreditaba el mixer del EXE. |

Los PASS iniciales acreditan sus contratos. El juego será jugable cuando el
bucle original y las escenas cumplan estos criterios; un contador de imports
o una pantalla de recursos no proporciona un porcentaje de port completado.

## Comprobaciones ejecutadas para la 74

Seis grupos en tests/startup_message_queue_test.cpp:

1. Cola vacía conserva el output MSG para flags 0..3.
2. FIFO, vínculo repetido sin reset, PM_NOREMOVE y PM_REMOVE.
3. HWND propio/NULL/-1 y filtros inclusivos de mensajes.
4. WM_QUIT no se excluye por el rango.
5. Copia fallida, punteros inválidos, thread/ventana ajenos y flags no soportados
   conservan la cola y no consumen eventos.
6. Capacidad de 32 y rechazo de productores inválidos sin perder eventos.

También volvieron a aprobar los seis grupos en tests/d3d8_frame_test.cpp:
geometría/muestreo/alfa; ONE/ZERO y modulación sobre X8; SRCALPHA por canal;
rechazo de geometría/alias sin escrituras; preparación ABGR/bandas;
bounds y RECT completo.

Compilados con g++ C++17, -Wall -Wextra -Werror y ASan/UBSan. Todos aprobaron.
Son patrones propios en PC: no ejecutan el EXE, los callbacks de la ventana,
el epílogo real de StartupServices ni la API de display de Vita. Los mensajes
publicados del test no son mensajes de entrada inyectados en el VPK.
Compilación VitaSDK e inspección ZIP/SFO/PNG aprobadas; ver
[el estado y SHA del paquete](STATUS_ITERATION74.md).
Present siguió sin ejecutarse en la 73: 0/8. No confundir con un fallo en la
API de presentación. La siguiente corrida debe validar el retorno de
PeekMessageA, el camino original a Present y cualquier frontera posterior.

## Cómo acelerar las siguientes iteraciones

- Reproducir en PC los datos capturados de cada nueva frontera y comprobar
  píxeles/contratos antes de otra instalación en Vita.
- Preparar varios contratos cuyo ABI/semántica se conocen del binario;
  detenerse en los que siguen sin implementar. No retornar éxito vacío.
- Usar la captura Windows existente como referencia de llamadas/estados;
  capturar otra pasada con apitrace cuando haga falta comparar una escena.
- Análisis estático automático de las diez funciones del arranque/presentación
  listadas en [la investigación](RESEARCH_EXE_ITERATION73.md).
- La build 74 copia al árbol WSL sólo fuentes que cambiaron: conserva timestamps
  y permite compilación incremental. Eliminó includes y ejecución del visor DAT,
  del título nativo independiente y del reproductor BGM de diagnóstico.
- Mantener en Vita los archivos originales: el propio EXE seguirá leyéndolos.

La 74 puede ejecutar y confirmar hasta ocho Present reales si ninguna frontera
anterior lo impide. Ese límite facilita una captura acotada; no son ocho tests
sintéticos ni una sesión jugable continua.
