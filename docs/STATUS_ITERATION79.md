# Iteración 79 — Texture Budget / 01.83

## Evidencia física de la 78

La 78 confirmó 203 Present reales y 203 señales/continuaciones timer/main,
con contextos preservados. Todos los hashes de scanout coinciden con la 77;
set/vblank/query RC 0 y matches:yes en cada frame. Logo edad 181, estado 220D,
fade de salida completo y último scanout negro 75895DC5.
22 preparaciones y 181 reutilizaciones de la caché; mediana de Present 35442 us.
La corrida tardó 74258537 us (74,26 s), sin fault/CPU limit, watchdog desarmado.

La corrección de propiedad pasó en hardware: ABC938 y su alias ABC940 fueron
destruidos, recuperando 2097152 bytes. Used quedó en 27413320. La asignación
que fallaba en la 77 tuvo éxito, seguida de otras dos imágenes. Hay 167
CreateTexture exitosas en total y 18 imágenes de opening cargadas.

La siguiente petición fue 512x512, levels 1, usage 0, formato 21/A8R8G8B8,
pool 1, out C0F6D8, retorno 605BBB. Pidió 1048576 bytes con 33459528 usados
de 33554432: solo quedaban 94904 bytes. El rechazo está registrado como
storage_budget; no fue bad_alloc ni agotamiento de handles.

MessageBoxA capturado: caption DGraphics-Error, texto japonés
«テクスチャの生成に失敗»: “falló la creación de la textura”. Mensaje y caption
terminaron en NUL, 22 y 15 bytes CP932 respectivamente. Esto confirma el
motivo que la versión anterior solo infería. No se devolvió un botón ficticio.
EIP B00760, ESP 9FEB70, EBP 9FEB84; SEH válido de dos niveles. 5785 servicios
D3D, 187 DInput, 199 DirectSound, 14131 imports main y 31 slices.
log.txt conserva inicializaciones y LoadEffect OK.

## Tamaño del recurso y presupuesto

El constructor original 4275D0/escena 0D usa data/system/opening.dat mediante
40BB80 y el número original de 24 imágenes. Se inspeccionaron los headers del
recurso local, sin incorporarlo al repositorio ni al VPK. No hay override de
opening en el th075b.dat inspeccionado.

Las primeras 18 imágenes coinciden en orden, dimensiones redondeadas y
formato con las 18 asignaciones físicas. Las 24 proyectan 15483392 bytes en
texturas de 32 bits. Sumados a los 27413320 bytes comunes retenidos, el total
es 42896712 bytes (40,91 MiB). Las seis imágenes pendientes incluyen la que
falló; sus contratos futuros de upload/dibujo aún no están verificados.

El límite software de 32 MiB era inferior a esa carga viva. La 79 permite
64 MiB, sin cambiar formatos, píxeles, EXE ni condiciones de escena. Es un
máximo acumulado de storage; no se reserva un vector de 64 MiB al arrancar.
Cada asignación sigue siendo real y puede fallar por heap. Se conservan las
liberaciones a último ref, los holds de bindings/targets/state blocks, el
staging de 4 MiB, el arena guest de 32 MiB y la capacidad de 512 handles.
No se evictan recursos vivos ni se devuelven texturas inexistentes.

## Memoria nativa y diagnóstico

El ELF enlazado de 78 y 79 usa el default SDK de 134217728 bytes (128 MiB)
para newlib. _newlib_heap_size_user continúa débil/sin definición del port;
_init_vita_heap selecciona ese valor. No se aumentó esa reserva. El comentario
histórico del runtime sobre 38 MiB describe otra aplicación que no se enlaza
aquí; se contrastó con el ELF y el
[inicializador de newlib de VitaSDK](https://github.com/vitasdk/newlib/blob/vita/newlib/libc/sys/vita/sbrk.c).

Se añaden observaciones en process_start, device_created, crecimiento por
tramos de 8 MiB, rechazos y startup_stop:

- startup_native_heap: reserved, managed, in_use, free_chunks y top_free,
  usando _get_vita_heap_size y mallinfo.
- startup_kernel_memory: free_user/CDRAM/PHYCONT mediante
  [sceKernelGetFreeMemorySize](https://docs.vitasdk.org/group__SceSysmemUser.html).
- startup_d3d8_storage_peak/summary: used, peak, máximo, recursos propietarios,
  aliases, handles asignados y capacidad.

El heap reservado ya está descontado de la memoria kernel libre; no se suman
esas cifras como una sola reserva disponible. mallinfo describe espacio
administrado por malloc, no garantiza un bloque contiguo suficiente ni cuenta
todo el heap aún no entregado por sbrk. Fragmentación y otros usuarios del
heap requieren medición física. Las lecturas no deciden éxito: vector::resize
y las guardas de presupuesto/capacidad conservan el fallo real.
Un query sysmem fallido registra rc y values:unavailable. Los campos de
mallinfo se interpretan según la
[documentación de newlib](https://sourceware.org/newlib/libc.html#mallinfo_002c-malloc_005fstats_002c-mallopt).

## Comprobaciones y paquete

34 grupos portables a -O2, -Werror y ASan/UBSan aprobados. Reproducción de
1295 llamadas reales de propiedad/estado de la 78: logo liberado y última
CreateTexture aceptada. Luego se proyectaron las cinco asignaciones restantes
desde headers locales, terminando en 42896712 bytes dentro del nuevo cap.
No se ejecutó x86, SDK, datos de uploads ni draws del juego en ese test.
Renderer a -O0/-O2 con outputs/digests iguales.

VitaSDK compiló sin advertencias en esta recompilación, -O2 -g -DNDEBUG y
FP estricto. nm confirma enlace de _get_vita_heap_size, mallinfo y sysmem.
ZIP CRC correcto, SFO 01.83/T075VITA1 y cuatro PNG indexados de ocho bits sin
entrelazado. Diagnósticos de memoria, carga y nuevos frames pendientes en Vita.

Build: iteration79-texture-budget-r1.
VPK local: artifacts/iteration79/Touhou75Vita-iteration79.vpk.
SHA256: 13560d090c17048a928899b98e4a3215dc79c3f77bd6d4383566500f4a4aa71b.

## Próxima corrida en Vita

1. Instalar 79/01.83, conservando TH075.exe y sus datos originales.
2. Esperar al diagnóstico: cap 100 s, watchdog 120 s. X sale.
3. Enviar iteration79.log, iteration79-runtime.log, iteration79-watchdog.log,
   log.txt y captura de STOP/PRESENT FRAMES/imagen.
4. Comprobar la asignación antes rechazada, nuevos frames/escena, pico real,
   heap, motivo de cualquier rechazo y texto de MessageBoxA si reaparece.

Seguimos en la carga original de opening después de la salida del logo.
La proyección de recursos no acredita que la escena ya dibuje ni que exista
un menú jugable. Próximos hitos: sus contratos de dibujo/servicio, transición,
entrada de menú, scheduler/audio guest y combate. Se conservan 240 frames/waits,
512 draws, 128 Mi píxeles cubiertos y 64 slices; no se alteran contadores del
EXE. El visor de música y sprites sigue retirado.
Véanse [comprobaciones](EXE_CHECKLIST_ITERATION79.md), [roadmap](ROADMAP.md) y
[proyectos/decompiladores revisados](RESEARCH_EXE_ITERATION73.md).
