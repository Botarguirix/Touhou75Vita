# Iteración 94 — memoria de selección y menú directo

## Causa observada en la Vita, iteración 93

La captura termina con 111/1800 Present, negro y USER32!MessageBoxA. La copia
local del log registra 101 Present/103 esperas, una pulsación Cross/Z y la
denegación de una textura de 2.097.152
bytes: había 66.386.760 bytes gráficos frente al límite de 67.108.864 (64 MiB).
El texto CP932 del diálogo es «テクスチャの生成に失敗» (falló la creación de una
textura), con título DGraphics-Error. No fue un vencimiento del límite CPU
ni una excepción nativa; el watchdog quedó desarmado. Todavía no se observó
el cambio de propietario a la escena de selección.

Los tamaños contiguos cargados coinciden con 19 de las 20 imágenes de
data/system/selectchar.dat. La imagen restante es 512×1024×4: proyecta
68.483.912 bytes gráficos (65,31 MiB). Es una proyección de almacenamiento,
no una ejecución de la escena ni una medición de su FPS.

En la parada, el heap nativo reservado era 128 MiB y usaba 80.906.240 bytes;
la diferencia respecto a gráficos era 14.519.480 bytes. El kernel reportaba
unos 60 MiB USER y 108 MiB CDRAM libres. Estas regiones son distintas del
heap reservado y sus cifras no prueban ausencia de fragmentación.

## Cambios 94

- El almacenamiento D3D8 admite hasta **96 MiB**, conservando su límite y
  el rechazo real por presupuesto o bad_alloc. No se falsea el éxito de
  CreateTexture ni se responde OK al diálogo. El heap sigue en 128 MiB y la
  capacidad de handles en 512; la textura restante cabe en la proyección.
  Una carga posterior o fragmentación aún puede agotar el heap.
- El primer propietario pasa directamente por el **constructor original
  del título 0043A020**, con asignación original de 0x3C bytes y estado
  0x2002. Se conservan inicialización común, comprobación de asignación,
  instalación del estado y destrucción del propietario. No se construye,
  carga ni dibuja el logo inicial; la apertura musical continúa omitida.
- Las tres capas de entrada del título empiezan en alpha 255, el endpoint
  original de 5 + 25×10. Se omite el desvanecimiento inicial, conservando
  selección, highlight, lectura de controles y salidas de escenas.
  Los parches se aplican sólo al EXE mapeado después de comprobar su SHA-256
  y los diez fingerprints; cambian 18 bytes. Un fallo de verificación,
  escritura o readback bloquea ejecución y provoca rollback comprobado.
- LiveArea, pantalla de carga, icono y botón de inicio se reconstruyen
  directamente del retrato original de **Alice**, frame 6 de selectchar.dat.
  Icono e inicio centran los límites visibles de alpha. No hay arte generado
  ni reutilización de PNG previamente cuantizado. El retrato y los cuatro
  PNG se mantienen fuera de Git; CI público omite el arte del juego.

La resolución interna sigue en 320×240 con interfaz guest 640×480, raster
serial y audio/input de la 93. No se activa el pool raw de las 87–89 ni se
modifican EXE/DAT en disco. La GPU y el objetivo de 60 FPS siguen pendientes.

## Comprobaciones locales

151 grupos C++ y 11 pruebas Python; ASan/UBSan, comparaciones O0/O2 del raster,
regresión de los 66.386.760 bytes observados seguida de la textura de 2 MiB,
agotamiento/reclamación del límite nuevo, ownership COM, rollback/readback
de los diez parches, destino del constructor y alpha final. También se
comprueban el hash del EXE y sus instrucciones originales, y la geometría
de centrado del arte con RGBA sintético.

Build Vita, identidad ELF/SELF/SFO, CRC del ZIP, memoria, imports y decode
completo PNG8 comprobados. No se ejecutó el EXE completo en PC; selección,
combate y FPS físicos de la 94 quedan pendientes.

## Paquete

- Title ID: T075VITA1; versión **01.98**.
- Nombre: **Touhou 7.5 Vita - Iteration 94 Selection Memory**.
- Build: **iteration94-selection-memory-r1**.
- VPK local: artifacts/iteration94/Touhou75Vita-iteration94.vpk.
- SHA-256: 6f55600857d7a07878e819a4d11a90e270fb76a0c1932969daad8dc706ac6106.

## Prueba física

1. Instalar el VPK 94 y comprobar nombre/diagnóstico ITERATION 94.
2. Comprobar Alice en LiveArea e icono, con inicio centrado.
3. Verificar entrada directa al menú y ausencia del fade de entrada.
4. Elegir Practice Mode con × y esperar la selección; si aparece, elegir
   personaje y escenario. No aceptar menú visible como combate jugable.
5. Exportar iteration94.log, iteration94-runtime.log e
   iteration94-watchdog.log y anotar la pantalla alcanzada, audio y FPS.

[Ruta jugable](PLAYABLE_PATH.md) · [60 FPS](PERFORMANCE_60FPS.md).
