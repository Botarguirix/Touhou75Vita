# Iteración 88 — Thread Startup

## Resultado físico posterior

La 88 pasó su chequeo del mutex, pero no arrancó el EXE. Su core parcial
localiza otro mutex nulo en libgcc emutls, antes de crear los workers:
pthread_once seguía omitido del enlace estático. La corrección de la 88
era insuficiente. [Iteración 89](STATUS_ITERATION89.md) documenta el nuevo
diagnóstico, chequeo TLS, paquete y arte. Rendimiento de la 88 no medido.

## Fallo físico de la 87

El usuario informa que el juego no alcanza a iniciar. La Vita genera
C2-12828-1 y un core psp2core-1791690503-0x0004bd22bb-eboot.bin.psp2dmp.
Se conservaron el core, los cuatro logs y el ELF con símbolos de esa
compilación fuera de Git, en artifacts/iteration88/input.

El core señala data abort 00030004 en el hilo principal T075VITA1.
PC físico 8121866A, código del módulo cargado en 81013000: offset 20566A,
equivalente a 8120566A en el ELF enlazado en 81000000.
addr2line y objdump identifican pthread_mutex_unlock, instrucción ldr r1,
[r3,#12], con r3=0. LR y pila sitúan pthread_cond_wait y
std::condition_variable::wait dentro de RasterWorkers::initialize,
raster_workers.h:94 de la 87. El primer hilo th075_raster_core1 existe;
no aparece el segundo. No se atribuye a una instalación fallida ni se
considera este resultado una medición del renderer paralelo.

## Causa y corrección

El gthr-posix instalado con GCC/VitaSDK usa una referencia débil a
pthread_cancel para activar std::mutex. La 87 no incluía ese símbolo:
crear hilos mediante sceKernelCreateThread no extrae pthread_cancel de
libpthread.a. El mutex C++ no se inicializaba ni bloqueaba; la espera de
condition_variable sí llamaba a pthread y terminaba leyendo el mutex nulo.
El desensamblado de la 87 muestra el marcador en cero antes del bloqueo.
La comprobación portátil con pthread del host no reprodujo esta condición
del enlace estático de VitaSDK.

La 88 fuerza --undefined=pthread_cancel en el enlace. No llama a cancel
ni cancela hilos. El símbolo queda definido en .text y el desensamblado
ARM muestra su dirección no nula y la llamada a pthread_mutex_lock antes
de esperar. La corrección también activa los mutex C++ del audio/runtime;
su comportamiento concurrente en Vita debe verificarse de nuevo.

Antes del EXE/JIT, startup_native_thread_support registra el resultado
real de __gthread_active_p y construye/bloquea/desbloquea un mutex. Si falla,
se rehúsa ejecutar el EXE y queda un diagnóstico. Las etapas create/start
y verificación de cada worker se vacían al archivo inmediatamente para
conservar el último checkpoint aunque haya una excepción nativa.
Actions comprueba con nm que pthread_cancel sea una definición de código.

Se conservan la división en tres bandas, núcleos de usuario 0/1/2,
validación previa, join antes de modificar recursos, sleep entre dibujos,
filtros, alpha y hashes desactivados de la 87. Preset de menú y controles
confirmados en la 86, límites de ejecución, EXE y DAT originales conservados.
No se integra GPU ni se declara ganancia de FPS sin hardware.

## Comprobaciones locales

107 grupos portables con -Werror, ASan/UBSan: los 106 de la 87 y uno nuevo
para el chequeo de activación/construcción/trylock/unlock. Siguen los
200000 samples bilineales, 500 strips, 1312 llamadas de ownership y
comparaciones completas contra el renderer serial con hilos host reales.
POINT/LINEAR/triángulos mantienen sus digests a -O0/-O2.

VitaSDK compiló con -O2 y FP estricto. nm/objdump verifican el símbolo y las
llamadas nativas corregidas; el ELF de la 87 sirve como testigo negativo.
Cuatro fingerprints del EXE coinciden. ZIP/CRC, versión SFO, PNG completos,
segmentos/relocaciones SELF y controles de memoria pasan. Las 14 bibliotecas
importadas y sus cantidades coinciden con la 87; SceCpu conserva su NID
de usuario comprobado. LiveArea continúa usando Alice desde DAT original.
El VPK no incluye EXE/DAT/audio/logs. Los ELF con símbolos se guardan para
futuras excepciones, fuera de Git.

Son pruebas locales y comprobación estática del enlace; no prueban por sí
solas el arranque, los tiempos, el audio o el combate en Vita.

## Paquete y prueba pendiente

- Nombre: Touhou 7.5 Vita - Iteration 88 Thread Startup.
- Title ID: T075VITA1. Versión: 01.92.
- Build: iteration88-thread-startup-r1.
- VPK: artifacts/iteration88/Touhou75Vita-iteration88.vpk.
- SHA-256: 0e64f35cb56ae75bcefddcc4508dd86036bfe9883a36db294e49ac42e0ca1a57

Instalar esta versión y comprobar primero startup_native_thread_support
passed y ambos startup_raster_worker verified:yes. Después verificar
entrada al menú, navegación, imagen, efectos y resumen de trabajos de
cada worker. Recoger iteration88.log, iteration88-runtime.log,
iteration88-watchdog.log y log.txt. Si vuelve C2-12828-1, conservar el
nuevo psp2dmp. Arranque/rendimiento de 88 y combate jugable: pendientes.

## Fuentes del diagnóstico

Formato de notas/mapeo del core: [vita-parse-core de xyzz](https://github.com/xyzz/vita-parse-core).
Activación de mutex C++: [gthr-posix de GCC](https://github.com/gcc-mirror/gcc/blob/master/libgcc/gthr-posix.h),
contrastado con las cabeceras GCC 15.2.0 instaladas de VitaSDK.
Espera condicional: [pthread-embedded de VitaSDK](https://github.com/vitasdk/pthread-embedded/blob/master/pthread_cond_wait.c).
No se incorporó código de esos proyectos a la aplicación.
