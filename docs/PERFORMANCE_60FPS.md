# Recursos de PC y objetivo: 60 FPS en Vita

La aceptación es **60 actualizaciones y frames completos por segundo**, con
presupuesto de aproximadamente **16,7 ms/frame**, respuesta de controles y
sonido estable en combate. Una captura del menú o una media de microbenchmark
no acepta el juego de pelea. Todavía no se alcanzó ni se verificó ese objetivo.

## Requisitos originales publicados para PC

| Recurso | Mínimo | Recomendado |
|---|---|---|
| CPU Pentium o compatible | 800 MHz | 1,2 GHz |
| RAM | 256 MB | 512 MB |
| Video DirectX 8 | 32 MB VRAM | 128 MB VRAM |
| Sistema | Windows XP/2000 | Los mismos |
| Instalación | Aproximadamente 1,1 GB | — |

El fabricante también destaca la importancia de la tarjeta gráfica.
[Requisitos oficiales de TH075](https://tasofro.net/touhou/index.html).
La interpretación para el port es que necesitamos trasladar las operaciones
D3D8 a GPU y reducir el coste de traducir x86 a ARM. GHz de Pentium, Cortex-A9
y PC actual no son una equivalencia directa de velocidad.

El equipo usado para las pruebas host tiene Core i5-1135G7, cuatro cores/ocho
hilos e Iris Xe, según consulta local de Windows. Nuestros microbenchmarks usan
CPU; no prueban DirectX ni simulan el rendimiento de la Vita a 500 MHz.
El EXE/DAT original está disponible localmente, pero estas pruebas no ejecutan
el juego completo en PC ni sustituyen una medición física en la consola.

## Recursos útiles revisados

- [TH075 de N0zoM1z0](https://github.com/N0zoM1z0/th075) identifica el mismo
  EXE japonés 1.11 y publica reconstrucción/comparación por funciones. El
  estado consultado declara 60 funciones exactas entre 4351 candidatos;
  no es todavía un motor completo recompilable para ARM.
- La copia local revisada identifica DrawTexturedQuad en 004029F0 (857 bytes)
  y DrawProjectedTriangleStrip en 00403200 (507 bytes), ambas exactas. Su
  fuente muestra queries GetSurfaceLevel/GetDesc/Release, UV/offsets, color y
  DrawPrimitiveUP. Esto ayuda a priorizar futuras sustituciones de wrappers
  y a validar el backend, conservando ABI, FPU, fingerprints y ownership.
  No se ha aplicado una sustitución x86→ARM de estas funciones en la 92.
- [apitrace](https://github.com/apitrace/apitrace) registra/reproduce llamadas
  gráficas y permite inspeccionar estados. El entorno ya contiene su build
  Win32 y wrapper D3D8. Una futura traza PC del mismo EXE permite comparar
  orden de draws, texturas, alpha/filtros y Present con el port.
- [Ghidra](https://github.com/NationalSecurityAgency/ghidra) ofrece análisis y
  decompilación del EXE. Sus resultados requieren reconciliar ABI, globals,
  llamadas y efectos; no producen automáticamente un port ARM completo.
- [VitaSDK GXM](https://docs.vitasdk.org/group__SceGxmUser.html) y
  [libvita2d](https://github.com/xerpi/libvita2d/blob/master/libvita2d/source/vita2d.c)
  son las referencias para contexto GPU, targets y presentación. Deben
  adaptarse al perfil D3D8 observado, alpha de RT, filtros y sincronización.
- [Box86](https://github.com/ptitSeb/box86/blob/master/docs/USAGE.md) documenta
  knobs de JIT. El runtime vendorizado ya compila ARMv7/NEON -O2 y usa
  BIGBLOCK=2, STRONGMEM=0, FASTNAN=1, FASTROUND=1 y X87DOUBLE=0. No se debe
  presentar como mejora activar algo ya activo, ni variar precisión sin
  comparación semántica. CALLRET/forward y funciones nativas requieren A/B
  con el runtime vendorizado y comprobación física de código/contextos.

## Qué falta, según la física 91

Se separan ciclos con exactamente un Present, y menú por vtable 00658308.
La inicialización, ciclos sin Present y ciclos incompletos no se usan para
inventar FPS. En menú hubo 337 ciclos válidos:

| Alcance | Media |
|---|---:|
| Ciclo timer + main | 114,57 ms (8,73 frames/s) |
| Dibujo CPU dentro del ciclo | 65,34 ms |
| Present/conversión/espera de scanout | 16,49 ms |
| Resto: guest + otras APIs + timer/logs | 32,74 ms |
| p95 / p99 del ciclo | 165,64 / 219,77 ms |
| Ciclos dentro de 16,667 ms | 0 de 337 |

No sumar el ciclo a sus componentes. Present incluye espera de refresco;
un backend GPU asíncrono debe permitir solapar preparación/dibujo sin añadir
una espera completa a un frame ya tardío. La GPU puede reducir el raster,
pero el resto observado supera por sí solo un presupuesto de 60 FPS.
No se infiere que toda esa diferencia sea tiempo de JIT.

## Iteraciones locales hechas y próximo orden

1. Referencia física 91 y replay con su header exacto c3ac4fa.
2. Candidato 92: alpha temprano, mezcla empaquetada y probes omitidos.
   Salidas idénticas; 384 geometrías, formatos y estados, oracle escalar,
   casos COM/readback y ASan/UBSan. Microbenchmarks host en tres perfiles.
3. Candidato final 92: telemetría del presupuesto, seis casos de límites y
   cuantiles; analizador Python con cinco pruebas. Nuevo VPK nativo y auditoría
   completa del paquete. La telemetría no cambia el ritmo del juego.
4. Siguiente: comparar 92 física, ubicar el coste real de guest/otras APIs,
   y portar quads/texturas a GXM con fallback CPU y captures comparativas.
5. Después: uploads por cambio, cache/ownership y batching donde el orden y
   estados lo permitan; sustituciones nativas de funciones calientes solo
   con fingerprints y replay de ABI/FPU/side effects del EXE identificado.
6. Verificar selección, combate y todos sus perfiles realmente solicitados.
   Audio en su worker nativo ya probado; lógica x86 secuencial conserva orden.

El pipeline final no se acepta por un 60 impreso: comprobar tiempo por frame,
velocidad de juego, menú/combate, input, sonido y estabilidad. El pool raw que
falló en 87–89 sigue desactivado; no es una prueba de que los cores sean inútiles.

## Analizador reproducible

Desde el repositorio, con Python:

```sh
python tools/analyze_frame_budget.py RUTA/iteration92.log --output budget92.json
python tests/test_frame_budget_analysis.py
```

El VPK 92 también escribe startup_60fps_budget, con frames/within/missed,
p50/p95/p99 y exclusiones explícitas. Para alcanzar la meta se necesitará
medir un combate sostenido y reducir las colas/tardanzas; la corrida actual
está acotada a 360 Present y no constituye una validación de combate.

## Prueba final preparada para Vita

Instalar **Iteration 92 Alpha Fast Path / 01.96** del VPK local, mantener los
mismos plugins/clocks de la 91, completar el menú y mover selección. Conservar
la captura/FPS y los cuatro logs. Revisar preflight, 360/360 Present,
startup_d3d8_alpha_summary y startup_60fps_budget. Comparar con 91 antes de
mezclar otros cambios. Si se entra a Practice/selección y surge una nueva
API o geometría, conservar esa frontera y el core si hay excepción.

Los 60 FPS físicos de la 92 y el combate están **pendientes**. Este es el
paquete final de la tanda local, preparado para la medición en consola.
