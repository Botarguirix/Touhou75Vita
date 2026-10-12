# Iteración 70 — Pipeline State / 01.74

## Evidencia física de la 69

La Vita atendió BeginStateBlock, SetViewport, EndStateBlock, CaptureStateBlock,
GetRenderTarget/GetDepthStencilSurface, SetRenderTarget y BeginScene. El destino
color pasó a la superficie ABC948, con profundidad ABC950 y almacenamiento
1024×1024; la viewport activa quedó en 640×480. Clear con flags 3 y Z=1 devolvió
HRESULT cero. Esto acredita la ejecución de esos contratos de almacenamiento;
no acredita píxeles producidos por el renderer original.

Después de SetRenderState(7, 0), el EXE pidió SetRenderState(14, 1), es decir,
ZWRITEENABLE=TRUE. La llamada permaneció sin ejecutar en trap BFC320.
El resultado fue checkpoint PASS sin fallo de CPU ni límite de ejecución.
Hubo 9666 llamadas principales, 2496 D3D, seis DInput, 199 DirectSound,
30 reanudaciones y 15641388 us; ESP final 9FEB68. log.txt terminó en
LoadEffect ...OK. BGM registró 246 bloques, cero late_fill_block y cierre correcto;
ese contador no mide directamente la salida acústica.

## Contratos preparados en conjunto

- Inicialización y lectura de los estados admitidos con sus valores por defecto.
  Un getter de un estado desconocido sigue siendo una frontera explícita.
- Profundidad, escritura Z, comparación Z, alpha test, factores de mezcla,
  culling, fill/shade y TextureFactor. Las combinaciones se conservan como estado;
  todavía no se aplican a triángulos rasterizados.
- Stage 0: operaciones/argumentos de color y alpha, direccionamiento, borde y
  filtros. El helper original utiliza valores de filtro 0/1/2/4; se conservan los
  enums 0..5, incluidos los heredados, sin afirmar que exista muestreo de textura.
- SetTexture/GetTexture para stage 0, con referencias propias del binding y una
  referencia adicional para el llamador de GetTexture. Los objetos deben ser
  texturas creadas por este bridge. El último Release mantiene la frontera de
  destrucción ya existente.
- Grabación, aplicación, captura y eliminación de bloques incluyen el binding
  de textura. EndStateBlock transfiere su referencia una sola vez; Capture cambia
  la referencia del bloque, Apply cambia la del dispositivo y Delete la libera.
- Clear borra únicamente la viewport activa, tanto color como profundidad,
  respetando pitch y límites reales. Evita convertir Z cuando el flag Z no está
  presente. SetViewport valida el destino actual también durante la grabación.

La familia se dedujo del desensamblado del mismo EXE y de la reconstrucción local
de sus helpers de estado. La llamada de escritura Z está en 4022C7; el helper de
mezcla 401F50 usa los ocho pares SrcBlend/DestBlend del juego. La reconstrucción
es parcial y no sustituye el motor completo. La interfaz y semántica de referencia
se contrastaron con el [header de D3D8 de Wine](https://raw.githubusercontent.com/wine-mirror/wine/master/include/d3d8.h)
y su [implementación del dispositivo](https://raw.githubusercontent.com/wine-mirror/wine/master/dlls/d3d8/device.c).

## Captura del primer dibujo

DrawPrimitiveUP (slot 72) reconoce su ABI de cinco argumentos contando `this`,
pero conserva el frame de llamada y se detiene sin ejecutar ni devolver éxito.
Registra callsite, topología, primitivas, FVF, stride, binding, destino y estados.
Con FVF 0x144 y stride 28 captura como máximo 48 vértices/1344 bytes, calcula un
hash y detalla hasta ocho vértices XYZ/RHW/ARGB/UV. Fuera de ese límite no copia
datos arbitrarios. Cada llamada COM reconocida registra además el retorno y ESP.

El helper original 4029F0 prepara cuatro vértices de 28 bytes y hace SetTexture
en 402D17, seguido de DrawPrimitiveUP en 402D3E: TRIANGLESTRIP, dos primitivas.
La futura captura física permitirá comprobar si ese es el primer dibujo real.
Podría aparecer antes otra llamada aún pendiente; la secuencia no se presume.

## Roadmap actual

| Área | Estado y próximo resultado necesario |
| --- | --- |
| Loader, CPU, archivos, heap y arranque | Probados hasta esta frontera del EXE; faltan servicios que aparezcan más adelante. |
| DAT y música nativa | Visor y reproducción funcionando en Vita; no sustituyen la lógica del juego. |
| Texturas y destinos de D3D8 | Carga y cambio de destino observados; familia de estado/binding de la 70 pendiente en hardware. |
| Primer dibujo del EXE | Capturar geometría y estados reales, implementar el subconjunto de rasterización observado. |
| Imagen original completa | Completar restauración de destinos/CopyRects si se solicita y Present; comprobar menú real. |
| Demo jugable | Entrada del menú/combate, scheduler, mixer DirectSound del EXE y lógica de partida; aún pendiente. |

El siguiente hito es un frame generado por los draws del EXE. Después vendrán
la pantalla inicial interactiva y una partida. Los DAT contienen recursos;
su extracción por sí sola no implementa la lógica ni garantiza un juego jugable.

## Paquete

VitaSDK compiló y empaquetó sin advertencias. Inspección: ZIP CRC correcto,
SFO 01.74/T075VITA1 y cuatro PNG indexados de 8 bits sin entrelazado. Se conserva
el icono del EXE; LiveArea identifica iteración 70 y versión 01.74.
No se ejecutaron nuevas pruebas unitarias. Los contratos nuevos requieren Vita.

Build: iteration70-pipeline-state-r1.
VPK local: artifacts/iteration70/Touhou75Vita-iteration70.vpk.
SHA256: ae26f9fafb8c68e3271c341eaaec97ad29b71073cc260abe3e2b23bb30a1fb44.

Enviar iteration70.log, iteration70-runtime.log, iteration70-watchdog.log,
iteration70-bgm.log, log.txt y captura. Revisar render_state, texture_binding,
stateblock_texture, clear, draw_boundary/draw_state/draw_stage/vertex y
startup_stop_import. El PASS del diagnóstico sigue sin acreditar boot completo
del juego, pantalla original ni gameplay.
