# Iteración 90 — revisión de regresión y recuperación serial

## Resultado físico recibido — 11 de octubre de 2026

La 90 recuperó el menú original y completó 360/360 Present sin excepción;
watchdog disarmed y límite esperado de 361 esperas. La recuperación serial
se confirma. Tiempos de dibujo: 86,69 s; tramo total: 142,44 s; trazas: 32 MB.
La siguiente prueba es la [91 de resolución reducida](STATUS_ITERATION91.md).
Combate completo y rendimiento suficiente siguen pendientes.

## Último checkpoint funcional

La referencia es la 86, commit a874ffd, versión 01.90: el usuario confirmó
menú original, navegación, controles y efectos limpios. Su log registra
272 Present y 5811 dibujos. Esto confirma ese checkpoint, no un combate
completo ni un port terminado.

| Versión | Cambio principal después de la 86 | Resultado físico |
|---|---|---|
| 86 / 01.90 | Menú directo, renderer serial, controles nativos | Menú y audio confirmados |
| 87 / 01.91 | Dos workers para repartir filas entre núcleos 0/1/2; condition_variable; menos hashes/logs; mayor límite de draws | Mutex nulo al esperar el primer worker |
| 88 / 01.92 | Fuerza pthread_cancel y verifica activación/mutex antes del VM | Mutex pasa; emutls del JIT falla por pthread_once omitido |
| 89 / 01.93 | Fuerza pthread_once/cond_destroy, comprueba TLS y renueva LiveArea | VM/JIT/inicialización original pasan; primer worker falla al esperar |
| 90 / 01.94 | Retira la ejecución de los workers; usa el renderer serial con hashes diagnósticos desactivados | Menú y 360 Present confirmados |

git diff a874ffd..a62974d no muestra cambios en el runtime WinVita/Box86,
dsound_bootstrap.h, dsound_pcm_mixer.h, dinput8_bridge.h o exe_boot_options.h.
El SHA del EXE original coincide y el preset de menú se valida correctamente
en el log de la 89. La nueva LiveArea es arte del paquete; ese checkpoint
ya fue superado antes del fallo observado.

Otras diferencias desde 86: set_raster_executor en el adaptador D3D8,
validación previa de geometría para repartir filas, hashes opcionales de
píxeles/scanout, dumps de estado solo en límites, consulta del core mediante
SceCpu y subida del límite de 6144 a 12288 dibujos. Los límites de tiempo,
frames, memoria gráfica y perfiles de filtros/alpha se conservan.

## Evidencia nueva de la 89

Los archivos recibidos y el ELF exacto se guardan fuera de Git, en
artifacts/iteration90/input. El core es completo: gzip con footer válido,
ELF de 3311036 bytes y final declarado también de 3311036 bytes.

iteration89.log confirma once, TLS y preflight passed, runtime inicializado,
smoke JIT/imports/heap/file/TEB/TLS/process passed y preset de menú aplicado.
log.txt alcanza DirectGraphics/DirectInput/DirectSound y SystemDataInit.
El último evento es la creación/inicio del primer worker para el primer
DrawPrimitiveUP grande. No se presenta todavía un frame.

El hilo que falla es th075_raster_core1, ID 40010167; data abort 00030004.
PC físico 81275E26, módulo en 8106F000: offset 206E26, dirección enlazada
81206E26. La instrucción ldr r5,[r4,#8] usa r4=0. Está en
pte_osSemaphoreCancellablePend, después de obtener el dato pthread del hilo.
La pila identifica sem_timedwait, pthread_cond_wait, condition_variable::wait
y RasterWorkers::entry. El principal y watchdog tienen reason=0.

El dato nulo es el contexto OS pthread que contiene el evento de cancelación,
no el mutex del fallo de la 87 ni la clave emutls del fallo de la 88.
VitaSDK instala ese contexto al crear un pthread; nuestros workers se
creaban directamente mediante sceKernelCreateThread. La espera cancelable
lo lee sin comprobar NULL. Se contrastó el desensamblado del ELF instalado
con [vita_osal.c de VitaSDK](https://github.com/vitasdk/pthread-embedded/blob/master/platform/vita/vita_osal.c)
y [pthread_create](https://github.com/vitasdk/pthread-embedded/blob/master/create.c).
Los mapas y desensamblados se conservan en artifacts/iteration90.

La integración multinúcleo introdujo la regresión. Las pruebas host usan
std::thread del sistema anfitrión, con soporte pthread completo; no
reproducían el contexto ausente de un hilo nativo de Vita. Que pasen los
tests de píxeles/TLS no certifica ese ciclo de vida en hardware.

## Recuperación de la 90

startup_probe no incluye, construye ni ejecuta RasterWorkers. El ejecutor
serial llama al mismo rasterize validado en la 86; desactiva únicamente los
hashes diagnósticos por píxel. El presenter conserva su comparación completa
de bytes para la caché y la confirmación del scanout. La LiveArea se codifica
desde los mismos maestros originales de la 89 con revisión 90 / versión 01.94.

Se mantiene el preflight nativo de once/TLS y su enlace corregido. Esto es
una recuperación dirigida del camino de dibujo, no un rollback completo
de todos los archivos de la 86. Permite comprobar el nuevo diagnóstico
aislando la espera de workers. La carga de CPU y la GPU siguen pendientes.

Los símbolos del ELF 90 y el código del SELF no contienen RasterWorkers,
th075_raster_core1/core2 ni el checkpoint de creación del pool.
startup_raster_policy identifica serial_recovery / raster_workers:disabled.

Antes de reactivar el pool, su creación/unión debe usar pthread registrado
o una sincronización nativa que no invoque las esperas cancelables de pthread.
Hay que validar en Vita inicio, espera, afinidad, TLS, jobs y teardown por
etapas. El pool experimental permanece como referencia host, desactivado.

## Comprobaciones y paquete

- 110 grupos portables con ASan/UBSan y -Werror: incluyen el ejecutor serial
  con POINT/LINEAR, A8/A1/X8, rectángulos/triángulos y rechazo sin escrituras.
  Coinciden todos los píxeles/counters/probes y se crean cero workers.
- 200000 muestras bilineales, 500 geometrías y 1312 llamadas de ownership;
  resultados de gráficos idénticos a -O0/-O2; pruebas de audio/input/TLS.
- VitaSDK -O2 con FP estricto, tres símbolos pthread definidos y ninguna
  función pthread referenciada sin resolver.
- ZIP/CRC, SFO 01.94, PNG8 completos, SELF/imports/control de memoria y
  cuatro fingerprints del EXE comprobados.
- VPK: artifacts/iteration90/Touhou75Vita-iteration90.vpk.
- Title ID: T075VITA1. Nombre: Touhou 7.5 Vita - Iteration 90 Serial Recovery.
- Build: iteration90-serial-recovery-r1. Tamaño: 1798336 bytes.
- SHA-256: 7dc2521b38e6d9eac9a05b136230b5c72e8eaae1abd7a7934e6cbadae3dc4313

Instalar la 90 y comprobar preflight, serial_recovery y Present mayor que
cero, entrada al menú, navegación y efectos. Recoger iteration90.log,
iteration90-runtime.log, iteration90-watchdog.log y log.txt, más captura.
Si vuelve a fallar, conservar el nuevo core. Arranque físico de la 90: menú
y 360 Present confirmados. Rendimiento suficiente y combate: pendientes.
