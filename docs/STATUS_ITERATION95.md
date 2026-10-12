# Iteración 95 — geometría al entrar a selección

## Evidencia física 94

Los archivos recibidos identifican build iteration94-selection-memory-r1,
watchdog iteration94 y los diez parches de arranque. La captura adjunta sigue
mostrando la 93 y no representa esa corrida. Los logs 94 completaron 46 Present
y 47 esperas; una pulsación Cross/Z produjo la salida del título y el cambio
a estado 0x2003, vtable 0065824C, update 00433470 y draw 00435A70. Es evidencia
de entrada lógica a la siguiente escena, sin confirmar su presentación completa.

El límite de memoria anterior quedó superado: gráficos alcanzaron 77.396.808
bytes (73,81 MiB), dentro de los 96 MiB nuevos; heap nativo en uso 91.916.456,
399/512 handles. No hubo allocation_denied ni MessageBoxA. El watchdog quedó
desarmado y no se alcanzó el límite CPU. La nueva frontera es DrawPrimitiveUP,
reject:quad, tras los primeros tres draws de selección. Su perfil conserva
POINT, triangle strip de dos triángulos y estados conocidos.

Los 45 ciclos medidos suman 15,202 s, incluyendo 8,639 s de carga/transición;
ninguno cumple 16,667 ms. Los 44 ciclos anteriores a esa carga promedian
149,167 ms (6,70 ciclos/s). No interpretar el contador 81 de la captura vieja
como 81 FPS de la 94. Audio nativo: 629 bloques, intervalo máximo 23,158 ms,
cero intervalos mayores que dos bloques; audibilidad requiere reporte humano.

## Diagnóstico y límite de evidencia

La 94 compactó los registros de vértices y no conservó sus palabras originales
al rechazar la geometría. Por ello no hay una reproducción exacta del draw físico.
El EXE original inicializa owner+0xA4 a 80.0 en 00433296; el draw posterior al
Rect 0,192,256,448 calcula escala 1 - offset/80.0 en 00435EC2 y aplica el
transform original 004115C0: (vértice-pivote)×escala+pivote. Al inicio esa escala
es cero. Esto hace probable una figura colapsada; la causa precisa del rechazo
físico todavía es una inferencia que los siguientes registros permitirán comprobar.

Direct3D permite triángulos de área cero sin generar fragmentos. La referencia
describe su uso para unir strips sin dibujar la conexión:
[Microsoft, Triangle Strips](https://learn.microsoft.com/en-us/windows/win32/direct3d9/triangle-strips).

## Cambios 95

- Dos triángulos con **área exactamente cero** se aceptan como draw válido
  sin fragmentos, muestreo, mezcla ni escritura. No se divide por cero ni
  se usa epsilon, snapping o modificación de coordenadas. Se conservan las
  comprobaciones previas de recursos, formatos, buffers, alias, UV, Z/RHW,
  diffuse y estados. Un strip parcialmente degenerado, cóncavo o plegado
  sigue siendo una frontera antes de escribir.
- El DrawPrimitiveUP original obtiene su epílogo COM normal sólo cuando el
  renderer valida ese no-op. Se registra cada caso y un resumen de geometría.
  No se ocultan draws desconocidos ni se avanza mediante éxito inventado.
- En una frontera real se guarda el paquete original: retorno, FVF, tipo,
  recursos, hash y hasta ocho vértices con sus bits float completos. Se
  reutilizan las palabras ya leídas; los draws normales conservan el log
  compacto y no se cambia globalmente su política. Así una siguiente frontera
  permite reproducción local precisa sin otra iteración sólo de instrumentación.

Se conservan menú directo sin logo/fade, 96 MiB gráficos/128 MiB heap,
320×240 internos, raster serial, audio, controles y arte DAT original de Alice.
No se activa el pool raw ni se cambian clocks/runtime/presets del EXE.

## Comprobaciones locales

158 grupos C++ y 11 pruebas Python con ASan/UBSan; comparación O0/O2 del
raster y del nuevo no-op. Casos: punto colapsado, línea horizontal/vertical/
oblicua, edge compartido repetido, degeneración parcial rechazada, validación
de buffers/alias/NaN/estado/formato, áreas pequeñas no nulas, executor serial,
epílogo COM y conservación de bits de vértices bajo log compacto.

Build Vita, ELF/SELF/SFO, ZIP, imports, memoria y decode PNG8 comprobados.
Las instrucciones y constantes del EXE original se comprobaron por hash y
fingerprint. No se ejecutó el EXE completo en PC ni se verificó combate físico.

## Paquete

- Title ID T075VITA1; versión **01.99**.
- Nombre **Touhou 7.5 Vita - Iteration 95 Selection Geometry**.
- Build **iteration95-selection-geometry-r1**.
- VPK local artifacts/iteration95/Touhou75Vita-iteration95.vpk.
- SHA-256: f517d018cf6a4d0f51d1e62105cf7c7972731769958bf78eb78dcfee8df30cf2.

## Prueba física

Instalar la 95 y comprobar su nombre y diagnóstico. Confirmar Practice Mode
con ×, esperar selección y, si aparece, elegir personaje y escenario. Exportar
iteration95.log, iteration95-runtime.log e iteration95-watchdog.log junto con
captura de la nueva corrida. Anotar pantalla alcanzada, controles, audio y FPS.
Se conservan 1800 Present/300 s y watchdog 330 s. Selección visible, combate
y 60 FPS siguen pendientes.

[Ruta jugable](PLAYABLE_PATH.md) · [60 FPS](PERFORMANCE_60FPS.md).

## Resultado físico recibido de la 95

El usuario confirmó selección visible en Practice y Story; ambas capturas
terminan después de confirmar personaje con GetTimeZoneInformation. Una
muestra 188 Present y otra 191; se recibió un único juego de logs, con 188
Present/189 esperas, cuatro Cross/Z y una salida del título a 0065824C.
Se procesaron 339 draws de área cero sin rechazo de geometría. No hay error
de asignación ni límite CPU; el watchdog se desarmó. El servicio de zona
horaria todavía no se ejecutó. Combate queda pendiente.

Gráficos: 77.396.808 bytes (73,81 MiB), 399/512 handles; heap en uso
91.916.456/134.217.728 bytes. Selección: 115 ciclos, media 135,287 ms,
7,39 ciclos/s; raster 64,623 ms, Present/captura 38,874 ms y resto 31,790 ms.
La mediana de todos los ciclos es 134,391 ms; 0/187 cumplen 16,667 ms.
La carga máxima de 8,404 s se analiza por separado.

Audio: 1205 bloques, máximo entre envíos 289,534 ms, un intervalo mayor que
dos bloques, lateness guest 15,858 s. No hay reporte de audibilidad para esta
corrida ni contador de underrun físico. Esta medición requiere seguimiento.

La selección visible y su navegación quedan comprobadas para las rutas
mostradas. La siguiente prioridad es GetTimeZoneInformation y la carga de
partida. [Nuevo roadmap](ROADMAP_AFTER_SELECTION95.md).
