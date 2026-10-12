# Comprobaciones para ejecutar TH075.exe en Vita

Estado tras recibir los logs físicos de la 74: PeekMessageA y un Present
nativo confirmados. La 75 continúa la espera del evento de frame mediante
el worker original; esa nueva integración necesita ejecución física.

## Orden de trabajo

| # | Comprobación | Criterio concreto | Estado |
|---|---|---|---|
| 1 | Identidad y carga PE32 | SHA del japonés 1.11, secciones/IAT y entrypoint correctos. | Confirmado en Vita. |
| 2 | ABI x86 y memoria | ESP/EIP, registros no volátiles, stdcall, TEB/FS/TLS, heap y accesos válidos. | Confirmado para los servicios usados; faltan casos futuros y ejecución sostenida. |
| 3 | Lectura de recursos original | Rutas, bytes leídos y offsets del DAT coinciden con lo solicitado por el EXE. | Carga inicial/texturas/effects alcanzada; falta cobertura completa. |
| 4 | Workers y sincronización | Threads bloquean/despiertan por eventos/timeout reales y permiten continuar el main. | Handoff y wakes parciales confirmados. 74 llegó al evento de frame AB2004; 75 añade hasta ocho continuaciones por el productor original. Validación física y scheduler continuo pendientes. |
| 5 | Texturas y estados D3D8 | Formato, pitch, Lock/Unlock, refs, bindings y viewport válidos. | Confirmados para la ruta inicial. |
| 6 | Primer dibujo del EXE | 307200 cubiertos, 223567 escritos y hash de destino EF895E7C. | Confirmado físicamente en la 72. |
| 7 | Cierre del frame D3DX | EndScene, restore de targets, ApplyStateBlock y Releases sin corrupción. | Confirmado físicamente en la 72. |
| 8 | Segundo quad y fade | ONE/ZERO, diffuse FF0F0F0F, fuente 1024², UV vertical 0..0.46875 y backbuffer X8. | Confirmado físicamente en la 73: 307200 escritos, 219077 cambiados, hash final 957A5E70 y EndScene correcto. |
| 9 | Present original | RECTs/ventana correctos, frame real enviado a scanout, vblank y readback coincidentes. | Confirmado en Vita 74: frame 1, retorno 40291E, limpieza 24, set/vblank/query RC 0, matches:yes y scanout B047B6F7. Repetición pendiente. |
| 10 | Bucle y entrada del menú | Procesar mensajes, reloj, entrada guest y aceptar una selección real. | PeekMessageA confirmó cola vacía/MSG/LastError y limpieza 24 en Vita 74. La siguiente espera de frame se aborda en 75. Entrada por mensajes, dispatch y selección del menú pendientes. |
| 11 | Gráficos completos y memoria sostenida | Geometrías, filtros, mezcla, superficies y destrucción sin fugas; hashes/imágenes comparados con PC. | Renderer rectangular parcial; backend completo pendiente. |
| 12 | Combate, sonido guest y datos persistentes | Entrar a combate, controles, lógica/colisión, mixer, tiempos, round y regreso al menú. | Pendiente. El audio nativo del visor no acreditaba el mixer del EXE. |

Los PASS iniciales acreditan sus contratos. El juego será jugable cuando el
bucle original y las escenas cumplan estos criterios; un contador de imports
o una pantalla de recursos no proporciona un porcentaje de port completado.

## Comprobaciones ejecutadas para la 75

Seis grupos nuevos en tests/startup_wait_policy_test.cpp:

1. Auto-reset consume una señal; SetEvent repetido no acumula permisos.
2. Manual-reset permanece señalado hasta ResetEvent.
3. Timeout real de 16 ms, INFINITE, reloj regresivo y cálculo sin overflow.
4. Perfil del wait observado: rechaza TIB, pila, retorno, evento o timeout ajenos.
5. Comparación del contexto: GPR, EIP, flags, FS y blob FPU/SIMD completo válido.
6. Límite de slices 32..64 y continuación sólo después de una señal.

Volvieron a aprobar los seis grupos de cola de mensajes y los seis de
raster/presentación documentados en la
[comprobación anterior](EXE_CHECKLIST_ITERATION74.md).
Compilados con g++ C++17, -Wall -Wextra -Werror y ASan/UBSan: 18 grupos aprobados.

Son patrones propios en PC. No ejecutan el EXE, los workers, los epílogos ABI,
la serialización real de la FPU de Box86 ni las APIs nativas de display/thread.
La nueva comprobación del cambio de contexto y SetEvent se ejecutará en Vita.
Compilación VitaSDK y ZIP/SFO/PNG aprobados; ver
[estado y SHA del paquete](STATUS_ITERATION75.md).

## Criterios de la siguiente corrida física

- startup_frame_wait_context=preserved tras cambiar al worker y volver.
- startup_event_signal=handle:0x00AB2004 con producer_tib:0x00760000.
- startup_frame_wait_signal=observed y startup_frame_wait_resume=passed.
- WAIT_OBJECT_0, evento auto-reset consumido, limpieza de 12 bytes y LastError conservado.
- Más de un Present original, o una nueva frontera explícita con su stack/EIP.
- Watchdog desarmado; sin fault, corrupción de contexto ni aceptación de un wait no soportado.

## Cómo acelerar las siguientes iteraciones

- Reproducir en PC los datos capturados de cada nueva frontera y comprobar
  píxeles/contratos antes de otra instalación en Vita.
- Preparar varios contratos cuyo ABI/semántica se conocen del binario;
  detenerse en los que siguen sin implementar. No retornar éxito vacío.
- Usar la captura Windows existente como referencia de llamadas/estados;
  capturar otra pasada con apitrace cuando haga falta comparar una escena.
- Análisis estático automático de las diez funciones del arranque/presentación
  listadas en [la investigación](RESEARCH_EXE_ITERATION73.md).
- La build 75 copia al árbol WSL sólo fuentes que cambiaron: conserva timestamps
  y permite compilación incremental. Eliminó includes y ejecución del visor DAT,
  del título nativo independiente y del reproductor BGM de diagnóstico.
- Mantener en Vita los archivos originales: el propio EXE seguirá leyéndolos.

La 75 puede ejecutar y confirmar hasta ocho Present reales si ninguna frontera
anterior lo impide. También permite hasta ocho continuaciones de la espera original de frame. Ese límite facilita una captura acotada; no son ocho tests
sintéticos ni una sesión jugable continua.
