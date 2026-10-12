# Iteración 89 — TLS Startup y arte de LiveArea

## Resultado físico posterior

La 89 supera once/TLS/preflight, VM/JIT y la inicialización original, pero
falla al esperar dentro del primer worker. Su core completo identifica
pte_osSemaphoreCancellablePend con el contexto OS pthread nulo de un hilo
creado directamente mediante sceKernelCreateThread. La corrección TLS
funcionó para el principal; no resolvió el ciclo de vida del pool.
[Iteración 90](STATUS_ITERATION90.md) revisa la regresión desde 86 y prepara
una recuperación serial. Rendimiento multinúcleo de la 89 no medido.

## Resultado físico de la 88

El usuario confirma que la aplicación no abre. iteration88.log contiene
solo la activación gthread y el mutex probado correctamente; el runtime
alcanza las reservas de memoria del JIT. log.txt es anterior a esta ejecución
y no demuestra inicialización del juego. No se recibió watchdog de la 88.

Se conservaron los archivos recibidos, el ELF coincidente y el análisis fuera
de Git, en artifacts/iteration89/input. El core .psp2dmp.tmp tiene un gzip
con footer completo, pero su ELF está incompleto: 972224 bytes presentes
frente a un final declarado de 2952160. Se leen completos los registros de
excepción, hilos, módulos y pila necesarios para localizar este fallo;
faltan datos de la cola. No se considera un volcado completo.

El hilo T075VITA1 muestra data abort 00030004, PC físico 812677BA. El módulo
está cargado en 81062000: offset 2057BA, dirección enlazada 812057BA.
pthread_mutex_unlock ejecuta ldr r1,[r3,#12] con r3=0. LR/pila y símbolos
sitúan __gthread_mutex_unlock, __emutls_get_address y internalDBGetBlock
de Box86. Todavía no existen los workers de raster ni el watchdog.

## Corrección del enlace y diagnóstico

La 88 activó gthread con pthread_cancel, pero libgcc emutls seguía usando
una referencia débil a pthread_once no extraída de libpthread.a. El
desensamblado muestra nop.w en lugar de la llamada: no inicializaba la
clave TLS ni su mutex. La comprobación del mutex de la 88 era insuficiente.

La 89 fuerza pthread_cancel, pthread_once y pthread_cond_destroy en el
enlace estático. nm confirma las tres definiciones de código y ninguna
función pthread referenciada queda débil/sin resolver. objdump comprueba
una llamada real a pthread_once en __gthread_once de libgcc. No se ejecuta
pthread_cancel; pthread_cond_destroy restaura también la limpieza del
mutex/condición usada por el pool. No son nuevas bibliotecas del sistema.

El preflight comprueba activación, mutex, dos llamadas a pthread_once con
un único callback, y escritura/lectura de TLS mediante thread_local
volatile. startup_native_thread_preflight=passed se emite solo al completar
todo. Cada worker comprueba su TLS antes de aceptar el pool. Se vacían al
disco la identidad 89 y los checkpoints anteriores al VM y al smoke JIT.

La división del raster en núcleos de usuario 0/1/2, perfil gráfico, audio,
menú directo, controles y límites de ejecución se conservan. GPU y ganancia
de FPS siguen pendientes; el arranque físico de la 89 no está confirmado.

## Arte solicitado

El nombre SFO/ejecutable de la 88 era correcto, pero el fondo y el template
reutilizados conservaban la revisión 87. Ahora CMake exige que content-rev,
procedencia e iteración/versión coincidan con el paquete antes de compilar.
Una configuración con el arte viejo fue rechazada como comprobación negativa.

A petición del usuario, image_gen integrado generó una portada pixel art
con Reimu/Marisa y un icono de Reimu. Los originales, prompts y procedencia
se guardan en artifacts/iteration89/artwork; la referencia de Vagante se
utiliza para el acabado visual. Cada PNG de LiveArea se deriva una vez de
esos originales, sin recomprimir imágenes de iteraciones anteriores.
El arte no lleva números de iteración; SFO, template y logs identifican la
89. prepare_generated_livearea.py genera los cuatro PNG8 opacos, conserva
proporciones y registra hashes/versión. Los maestros quedan fuera de Git.

## Comprobaciones locales

- 109 grupos portables con -Werror y ASan/UBSan. Tres grupos cubren preflight,
  callback único y aislamiento TLS entre el principal y dos hilos host.
- Comparación serial/paralela, 48 strips concurrentes, 500 geometrías de
  referencia, 200000 samples bilineales y 1312 llamadas de ownership.
- POINT/LINEAR/triángulos producen resultados idénticos a -O0 y -O2.
- VitaSDK con -O2 y FP estricto; símbolos y llamada ARM pthread_once verificados.
- ZIP/CRC, SFO 01.93, cuatro PNG8 con decodificación completa, límites y
  descompresión SELF; 14 bibliotecas importadas y controles de memoria iguales a 88.
- Cuatro fingerprints del EXE y diez bytes del preset mapeado comprobados;
  los archivos originales del usuario siguen intactos y fuera del VPK.

Las pruebas host y el enlace estático no demuestran arranque, audio ni
rendimiento en Vita. El core de la 88 tampoco mide el renderer paralelo.

## Paquete y siguiente prueba

- Nombre: Touhou 7.5 Vita - Iteration 89 TLS Startup.
- Title ID: T075VITA1. Versión: 01.93.
- Build: iteration89-tls-startup-r1.
- VPK: artifacts/iteration89/Touhou75Vita-iteration89.vpk.
- Tamaño: 1803127 bytes.
- SHA-256: 929de57d1e81b9d44876a5883de7a3cf6a487976a22568bf2c3a33f8665cff8b

Primero comprobar once/TLS/preflight passed, runtime inicializado y smoke
JIT superado. Después ambos workers verified:yes y tls:passed, menú,
controles, efectos y resumen de trabajo por núcleo. Entregar iteration89.log,
iteration89-runtime.log, iteration89-watchdog.log si existe, log.txt y una
captura. Ante nueva excepción, conservar el psp2dmp o .tmp y el ELF 89.

## Recursos utilizados

Formato del core: [vita-parse-core](https://github.com/xyzz/vita-parse-core).
Se contrastaron el emutls y gthr-posix instalados de GCC/VitaSDK con
[libgcc emutls](https://github.com/gcc-mirror/gcc/blob/master/libgcc/emutls.c)
y [gthr-posix](https://github.com/gcc-mirror/gcc/blob/master/libgcc/gthr-posix.h).
El enlace usa el [pthread-embedded de VitaSDK](https://github.com/vitasdk/pthread-embedded).
