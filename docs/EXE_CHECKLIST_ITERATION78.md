# Comprobaciones para ejecutar TH075.exe en Vita

La 77 confirmó 203 Present/continuaciones, edad original 181 y fade de salida
completo. La carga siguiente falló por presupuesto de texturas y llamó a
MessageBoxA. La 78 corrige referencias/liberación; hardware 78 pendiente.
Todavía faltan los píxeles de la siguiente escena, menú y combate interactivos.

## Orden de trabajo

| # | Comprobación | Criterio concreto | Estado |
|---|---|---|---|
| 1 | Identidad y carga PE32 | SHA del japonés 1.11, secciones/IAT y entrypoint correctos. | Confirmado en Vita. |
| 2 | ABI x86 y memoria | ESP/EIP, registros no volátiles, stdcall, TEB/FS/TLS, heap y accesos válidos. | Confirmado para los servicios usados; faltan casos futuros y ejecución sostenida. |
| 3 | Lectura de recursos original | Rutas, bytes leídos y offsets del DAT coinciden con lo solicitado por el EXE. | Carga inicial/texturas/effects alcanzada; falta cobertura completa. |
| 4 | Workers y sincronización | Threads bloquean/despiertan por eventos/timeout reales y permiten continuar el main. | 77 confirmó 203 señales del worker 12, waits auto-reset consumidos y contextos preservados. Scheduler continuo de todos los workers pendiente. |
| 5 | Texturas y estados D3D8 | Formato, pitch, Lock/Unlock, refs, bindings y viewport válidos. | Ruta inicial confirmada; 77 expuso retención incorrecta del logo. 78 corrige destrucción/refcount, comprobada en PC y pendiente en Vita. |
| 6 | Primer dibujo del EXE | 307200 cubiertos, 223567 escritos y hash de destino EF895E7C. | Confirmado físicamente en la 72. |
| 7 | Cierre del frame D3DX | EndScene, restore de targets, ApplyStateBlock y Releases sin corrupción. | Confirmado físicamente en la 72. |
| 8 | Segundo quad y fade | ONE/ZERO, diffuse FF0F0F0F, fuente 1024², UV vertical 0..0.46875 y backbuffer X8. | Confirmado físicamente en la 73: 307200 escritos, 219077 cambiados, hash final 957A5E70 y EndScene correcto. |
| 9 | Present original | RECTs/ventana correctos, frame real enviado a scanout, vblank y readback coincidentes. | 77 confirmó 203 frames, RC 0 y matches:yes; primeros dieciséis hashes iguales a 76. 22 preparaciones y 181 reutilizaciones exactas de conversión. |
| 10 | Bucle y entrada del menú | Procesar mensajes, reloj, entrada guest y aceptar una selección real. | 77 alcanzó edad 181, estado 220D, fade de salida y carga siguiente. Entrada y selección real del menú pendientes. |
| 11 | Gráficos completos y memoria sostenida | Geometrías, filtros, mezcla, superficies y destrucción sin fugas; hashes/imágenes comparados con PC. | Renderer rectangular parcial; backend completo pendiente. |
| 12 | Combate, sonido guest y datos persistentes | Entrar a combate, controles, lógica/colisión, mixer, tiempos, round y regreso al menú. | Pendiente. El audio nativo del visor no acreditaba el mixer del EXE. |

Los PASS iniciales acreditan sus contratos. El juego será jugable cuando el
bucle original y las escenas cumplan estos criterios; un contador de imports
o una pantalla de recursos no proporciona un porcentaje de port completado.

## Comprobaciones ejecutadas para la 78

34 grupos propios a -O2, -Wall -Wextra -Werror y ASan/UBSan:

- Los 23 grupos anteriores de raster/conversión, mensajes, waits/contexto y
  caché/miniatura, descritos en la [lista de la 77](EXE_CHECKLIST_ITERATION77.md).
- Seis grupos sobre la clase D3D8Storage usada por Vita: superficies de nivel
  reenvían referencias; el padre sobrevive al Release del caller mientras
  existe una superficie externa; destrucción a cero recupera presupuesto e
  invalida interfaces; bindings y transfer/apply/delete de state blocks retienen
  correctamente; render target mantiene padre hasta ser reemplazado; superficies
  implícitas sobreviven; outputs inválidos no incrementan refs y un último ref
  locked se rechaza; liberar logo de 2 MiB permite la asignación de 1 MiB.
- Cinco grupos de observación de strings: NULL/punteros inválidos sin lectura,
  bytes CP932 exactos con NUL, fallo de lectura con prefijo válido, límite del
  arena y truncamiento tras 256 bytes.

Además se reprodujeron 1.286 llamadas de propiedad/estado de los argumentos
del log físico 77 contra D3D8Storage: CreateTexture/Depth, GetSurfaceLevel,
AddRef/Release, render/depth targets, bindings y state blocks. El logo ABC938
se destruye y la última CreateTexture, antes 8876017C, devuelve éxito.
La reproducción omite draws, uploads, datos de recursos, SDK y ejecución x86;
no acredita el nuevo arranque completo. La traza se conserva fuera de Git.

El renderer a -O0/-O2 produjo salidas y digests iguales. Los tests conservan
asserts y sus shims SDK fallan si se llega a una ruta nativa inesperada.
La compilación Vita usa -O2 -g -DNDEBUG y FP estricto; las guardas de producción
son explícitas. ZIP CRC, SFO 01.82/T075VITA1 y cuatro PNG indexados sin
entrelazado aprobaron. Véase [estado/SHA](STATUS_ITERATION78.md).

Los tests nuevos se pueden reproducir en un host con g++ desde la raíz:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -Itests/support -Ithird_party/winvita/src tests/d3d8_storage_test.cpp third_party/winvita/src/runtime/cpu.cpp -o /tmp/d3d8_storage_test
/tmp/d3d8_storage_test
g++ -std=c++17 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer tests/guest_string_probe_test.cpp -o /tmp/guest_string_probe_test
/tmp/guest_string_probe_test
```

## Criterios de la siguiente corrida física

1. Build iteration78-resource-lifetime-r1, pantalla 78 y versión 01.82.
2. Hashes/fases de logo iguales a 77, señales/waits reales y contexto preservado.
3. Tras el fade, startup_d3d8_resource_destroyed para ABC938 con bytes:2097152
   y disminución de used. La asignación siguiente de 512x512 debe tener éxito.
4. Registrar nuevos recursos, vtable/escena, draws y Present tras frame 203.
   Un servicio o perfil desconocido se conserva como frontera explícita.
5. Si hay otra asignación rechazada, distinguir storage_budget, native_heap
   y handle_capacity. Si sigue MessageBoxA, recuperar text/caption raw_CP932
   del log; no asumir el mismo fallo anterior ni devolver OK artificial.
6. Watchdog desarmado, sin fault/corrupción. Cap: 240 frames/waits, 100 s,
   watchdog 120 s. La parada por cap no equivale a arranque completo.

## Cómo acelerar las siguientes pruebas

Usar contratos y argumentos de la primera frontera real para reproducir varios
casos de propiedad, ABI y errores en PC antes de construir. Mantener referencias
traducidas y alcances explícitos para geometrías, llamadas y workers no probados.
Esto reduce viajes de instalación; el siguiente código original solo se conoce
cuando el EXE continúa y los nuevos logs/estados lo registran. No se altera su
contador de escenas ni se fabrican respuestas para avanzar.
Los proyectos externos siguen evaluándose por compatibilidad y licencia según
[la investigación](RESEARCH_EXE_ITERATION73.md). Los DAT contienen recursos;
su presencia por sí sola no sustituye la lógica ejecutable del juego.
