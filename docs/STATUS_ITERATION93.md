# Iteración 93 — entrada a opciones y seguimiento de escenas

## Evidencia física 92

La captura muestra el menú y 7–10 FPS instantáneos. El usuario informa negro
al confirmar con × y después regreso al diagnóstico. El log adjunto termina
por el límite de 361 esperas/360 Present, sin excepción, sin límite CPU y con
watchdog desarmado. Sólo contiene logo y título (vtables 00657BE0/00658308),
11 pulsaciones arriba y 15 abajo; no contiene una pulsación Z/Cross ni salida
del título. Por eso todavía no identifica la causa de la transición negra
descrita. No se acepta esa falta de evidencia como prueba de que no ocurrió.

Tiempo total: 49,215 s. Los 7835 draws consumieron 19,421 s, frente a 23,912 s
en la 91. La media de 337 ciclos de menú fue 121,707 ms (8,216 frames/s):
dibujo 54,122 ms, Present/captura 35,147 ms, resto 32,438 ms. Ninguno estuvo
dentro de 16,667 ms. Las selecciones/input varían entre corridas; la reducción
de tiempo de raster no demuestra una mejora equivalente del FPS total.

La resolución nativa ya era 320×240 en la 91 y se conserva en 92/93. El EXE
sigue viendo 640×480 y los DAT/texturas originales mantienen su formato.

## Cambios 93

- DirectInput consulta hasta 64 muestras por Peek no bloqueante, ordenadas
  por timestamp sin heap ni worker. Un toque de acción pulsado y soltado entre
  polls se entrega una vez. Direcciones/analógico conservan el último estado.
  No se reproducen muestras antiguas ni eventos anteriores a Acquire.
  Varias pulsaciones de la misma acción dentro de un poll se agrupan; el
  historial de 64 muestras no puede recuperar una entrada ya expulsada.
- La prueba permite hasta 1800 Present/1801 esperas o 300 s, con watchdog
  independiente de 330 s. Máximo de 65536 draws y 8192 yields de audio; los
  límites de memoria, píxeles, geometría, servicios y progreso siguen activos.
- Se registran cambios de object/vtable/update/draw y número de salidas del
  título. El resumen de input cuenta Cross/Circle y toques recuperados. La
  pantalla identifica los límites normales de la prueba como TEST FRAME/TIME
  LIMIT, conservando la frontera real de APIs/contratos para otros casos.
- La miniatura del diagnóstico calcula coordenadas por columna/fila y copia
  filas repetidas. Para un frame 320×240 se calculan 420 coordenadas, frente
  a 86400 en el bucle anterior. La imagen resultante coincide con el oracle
  de la 92. Esto reduce trabajo auxiliar; no sustituye la GPU.
- Los totales del presupuesto de 60 FPS abarcan todos los ciclos válidos.
  Los cuantiles nativos siguen acotados a los primeros 512. El analizador de
  logs calcula todas las muestras y separa cada escena observada.

## Validación local

147 grupos C++ y 6 pruebas Python; ASan/UBSan, comparaciones O0/O2 del raster,
COM/readback, input breve/reacquire/error, límites de sesión, escena y preview
escalar. Build Vita, SELF/SFO/imports, memoria y PNG8 comprobados. Estas pruebas
no ejecutan el EXE completo en PC ni demuestran un combate físico.

Se mantienen renderer serial, alpha de la 92, fuentes 320×240, audio nativo,
presets/fingerprints del menú y runtime Box86/WinVita. No se activa el pool
raw que produjo excepciones en 87–89. LiveArea parte de los maestros originales.

## Paquete

- Title ID: T075VITA1; versión **01.97**.
- Nombre: **Touhou 7.5 Vita - Iteration 93 Menu Handoff**.
- Build: **iteration93-menu-handoff-r1**.
- VPK local: artifacts/iteration93/Touhou75Vita-iteration93.vpk.
- SHA-256: a86a21552322f1db25ed467309d0cd54f48c7857e3f43b31b3f46bf92c002d33

## Prueba física

1. Confirmar ITERATION 93 / 01.97 y probar primero **Practice Mode**.
2. Pulsar × una vez; si no responde, mantenerlo aproximadamente medio segundo
   y soltar. Si aparece selección, elegir personaje y confirmar. ○ es X del
   teclado original (cancelar), × es Z (confirmar).
3. Si aparece negro, anotar la opción elegida y si hay sonido/controles.
   Conservar el diagnóstico o el core si hay excepción. El nuevo seguimiento
   debe mostrar si el guest recibió Z y cambió de escena o pidió una API nueva.
4. Adjuntar iteration93.log, iteration93-runtime.log, iteration93-watchdog.log,
   log.txt y captura. El límite normal ahora es 1800 frames o cinco minutos.

La siguiente frontera es entrada real a selección/carga/combate y sus APIs.
Después se validará dibujo por GXM y reducción del coste guest. **60 FPS y
combate jugable siguen pendientes.** [Recursos y fases](PLAYABLE_PATH.md).
