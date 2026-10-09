# Comprobaciones para ejecutar TH075.exe en Vita

Estado al recibir los logs físicos de la 72. La 73 incorpora los siguientes
contratos, pero todavía necesita ejecución en la consola.

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
| 8 | Segundo quad y fade | ONE/ZERO, diffuse FF0F0F0F, fuente 1024², UV vertical 0..0.46875 y backbuffer X8. | Implementado 73; comprobación de píxeles en PC aprobada; Vita pendiente. |
| 9 | Present original | RECTs/ventana correctos, frame real enviado a scanout, vblank y readback coincidentes. | Implementado 73 con límite de ocho frames; llamada nativa/ABI en Vita pendientes. |
| 10 | Bucle y entrada del menú | Procesar mensajes, reloj, entrada guest y aceptar una selección real. | Pendiente; registrar la siguiente frontera antes de completar contratos. |
| 11 | Gráficos completos y memoria sostenida | Geometrías, filtros, mezcla, superficies y destrucción sin fugas; hashes/imágenes comparados con PC. | Renderer rectangular parcial; backend completo pendiente. |
| 12 | Combate, sonido guest y datos persistentes | Entrar a combate, controles, lógica/colisión, mixer, tiempos, round y regreso al menú. | Pendiente. El audio nativo del visor no acreditaba el mixer del EXE. |

Los PASS iniciales acreditan sus contratos. El juego será jugable cuando el
bucle original y las escenas cumplan estos criterios; un contador de imports
o una pantalla de recursos no proporciona un porcentaje de port completado.

## Comprobaciones ejecutadas para la 73

Seis grupos locales en tests/d3d8_frame_test.cpp, compilados con g++ C++17,
-Wall -Wextra -Werror y AddressSanitizer/UndefinedBehaviorSanitizer:

1. Geometría y muestreo del quad físico 640×480, bordes y alpha test.
2. Modulación del fade y ONE/ZERO sobre X8R8G8B8, con alfa de fuente parcial.
3. Mezcla SRCALPHA/INVSRCALPHA en los cuatro canales.
4. Geometría inválida y alias dejan el destino intacto.
5. Preparación ARGB→ABGR, proporción 725×544 y bandas negras.
6. Bounds del backbuffer y RECT completo del cliente.

Todos aprobaron. Los inputs locales son patrones propios: estas pruebas no
son una comparación completa con el renderer de Windows ni ejecutan el EXE.
La compilación Vita y la inspección ZIP/SFO/PNG también aprobaron.
Present nativo, su limpieza stdcall y rendimiento físico necesitan los logs 73.

## Cómo acelerar las siguientes iteraciones

- Reproducir en PC los datos capturados de cada nueva frontera y comprobar
  píxeles/contratos antes de otra instalación en Vita.
- Preparar varios contratos cuyo ABI/semántica se conocen del binario;
  detenerse en los que siguen sin implementar. No retornar éxito vacío.
- Usar la captura Windows existente como referencia de llamadas/estados;
  capturar otra pasada con apitrace cuando haga falta comparar una escena.
- Análisis estático automático de las diez funciones del arranque/presentación
  listadas en [la investigación](RESEARCH_EXE_ITERATION73.md).
- La build 73 copia al árbol WSL sólo fuentes que cambiaron: conserva timestamps
  y permite compilación incremental. Eliminó includes y ejecución del visor DAT,
  del título nativo independiente y del reproductor BGM de diagnóstico.
- Mantener en Vita los archivos originales: el propio EXE seguirá leyéndolos.

La 73 puede ejecutar y confirmar hasta ocho Present reales si ninguna frontera
anterior lo impide. Ese límite facilita una captura acotada; no son ocho tests
sintéticos ni una sesión jugable continua.
