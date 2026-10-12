# Iteración 72 — First EXE Quad / 01.76

## Evidencia física de la 71

SetRect escribió y verificó RECT 0,0,640,480; SetTexture vinculó la textura
ABC938. El EXE se detuvo antes de ejecutar DrawPrimitiveUP, call 402D3E y
retorno 402D44, sin fallo de CPU ni límite. El desensamblado identifica el
recurso como data/system/logo.dat, cargado desde el constructor 425490.

La captura contiene un TRIANGLESTRIP de dos primitivas, FVF 144, stride 28:

| Vértice | X,Y | Z,RHW | Color | U,V |
|---|---|---|---|---|
| 0 | -0.5,-0.5 | 0.5,1 | FFFFFFFF | 0,0 |
| 1 | 639.5,-0.5 | 0.5,1 | FFFFFFFF | 0.625,0 |
| 2 | -0.5,479.5 | 0.5,1 | FFFFFFFF | 0,0.9375 |
| 3 | 639.5,479.5 | 0.5,1 | FFFFFFFF | 0.625,0.9375 |

La fuente tiene 1024×512 píxeles A8R8G8B8, pitch 4096 y hash FNV-1a
0757B5D1. Contiene 300721 píxeles con alfa cero, 223567 con alfa 255 y
ninguno con alfa intermedia. Destino: superficie ABC948 del recurso ABC010,
1024×1024 A8R8G8B8; viewport 640×480, limpiada a 0F000000.
Se observaron point sampling, WRAP U/V, MODULATE, profundidad desactivada,
alpha test GEQUAL 1 y mezcla SRCALPHA/INVSRCALPHA.

Hubo 9674 llamadas principales, 2503 D3D, seis DInput, 199 DirectSound,
30 reanudaciones y 13511533 us. Watchdog disarmed; log.txt terminó con
LoadEffect ...OK. Círculo cambió contenedor/frame y las exportaciones funcionaron.
BGM cerró correctamente: 1842 bloques y seis late_fill_block en unos 86.6 s;
ese contador no demuestra por sí solo un corte acústico.

## Integración nueva

- DrawPrimitiveUP ejecuta el perfil observado sobre almacenamiento propio.
  Valida escena, FVF, geometría rectangular, UV separables, RHW, estados,
  formatos, viewport, pitch, límites, locks y ausencia de alias antes de escribir.
- Muestrea POINT/WRAP, modula color, aplica alpha test y mezcla, y escribe
  píxeles reales. Fuentes 21/25; destinos 21/22. X8R8G8B8 conserva RGB y
  normaliza el byte X a FF en Clear y en los píxeles aceptados.
- Conserva fronteras para otras geometrías, filtros, shaders, operaciones de
  mezcla y estados no implementados. No devuelve éxito para esos dibujos.
- Registra covered, alpha_rejected, written, changed, hashes de fuente/destino
  y sondas del primer/último píxel. El quad de la 71 debe cubrir 307200 píxeles,
  desde texel 0,0 hasta 639,479, si vuelve a recorrer esa misma ruta.
- Limita esta fase a 64 dibujos y 16777216 píxeles cubiertos por ejecución.
  Se comprueba el presupuesto antes de cada escritura; no son pruebas sintéticas.
- Mantiene una captura fija de 260×180 fuera del objeto D3D temporal y la
  muestra inicialmente como EXE DRAW CAPTURE. La conversión ARGB→ABGR
  muestra RGB opaco; no altera el alfa del recurso original ni ejecuta Present.

El rasterizador usa los centros enteros de píxel y excluye bordes derecho e
inferior. La geometría -0.5 corresponde al ajuste descrito por Microsoft en
[rasterización](https://learn.microsoft.com/en-us/windows/win32/direct3d9/rasterization-rules)
y [mapeo texel/píxel](https://learn.microsoft.com/en-us/windows/win32/direct3d9/directly-mapping-texels-to-pixels).
Los factores de mezcla siguen
[D3DBLEND](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dblend).
Es código propio; no se incorporó código externo ni recursos del juego en Git.

Para la textura física de la 71, alfa binaria y diffuse blanco producen copia
opaca o rechazo. El redondeo de alfa/color parciales y la conversión de formato
25, además del destino 22, necesitan comparación posterior con el original.
Este renderer acotado no establece equivalencia de un backend completo.

## Controles y siguiente frontera

Círculo abandona la captura inicial y elige DAT/frame aleatorios. Izquierda y
Derecha recorren frames. Start muestra el DAT actual antes de exportarlo,
evitando exportar un recurso oculto bajo la captura EXE. Triángulo cambia BGM,
Cuadrado reinicia y X sale. La música nativa no cambia su ruta de reproducción.

Después del primer dibujo, el original cierra el frame D3DX: EndScene,
restauración de destino/profundidad, ApplyStateBlock del viewport y Releases.
CopyRects es condicional; la ruta directa observada en la 71 lo omite.
El siguiente log determinará si llega a otro quad, a otro servicio o a Present.
Present sigue sin implementar. Menú original, gameplay y mixer guest pendientes.

## Paquete

Build: iteration72-first-exe-quad-r1.
VPK: artifacts/iteration72/Touhou75Vita-iteration72.vpk.
SHA256: 95a116913f8e6f5b722c20c7c8158d3143e9769cbf7ea733c535c1a73376de41.

VitaSDK compiló sin advertencias. ZIP CRC correcto; SFO 01.76/T075VITA1;
cuatro PNG indexados de 8 bits, sin entrelazado. LiveArea conserva el icono
del EXE y muestra iteración 72/version 01.76. Revisión estática de ABI,
rasterizador, captura y controles; diff --check limpio.
No se añadieron ni ejecutaron nuevas pruebas unitarias. Prueba física de la 72
pendiente; PASS del diagnóstico sólo acredita el checkpoint alcanzado.

Enviar iteration72.log, iteration72-runtime.log, iteration72-watchdog.log,
iteration72-bgm.log, log.txt y una foto del panel inicial antes de pulsar Círculo.
Revisar draw=executed, hashes, captura, restauración y startup_stop_import/EIP.
