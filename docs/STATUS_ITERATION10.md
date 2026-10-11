# Iteración 10 — estado básico del hilo y proceso

Estado: **validada en la Vita física del usuario** con `iteration10.log`, `iteration10-runtime.log` y una captura de las ocho pruebas en PASS. El build identificado es `iteration10-winvita-teb-fs-tls-r1`. Pasaron identidad, CPU, IAT, heap, archivo, TEB/FS, TLS y proceso. El framebuffer se presentó y se retiró sin error; la salida fue con X. El entry point del juego sigue sin ejecutarse.

La evidencia resumida se conserva en [hardware/iteration10-r1/result-excerpt.txt](hardware/iteration10-r1/result-excerpt.txt). El [plan de arranque](STARTUP_PROBE_PLAN.md) identifica la primera API del camino inicial todavía sin implementar.

## Corrección de la generación del VPK

La [ejecución 26](https://github.com/Botarguirix/Touhou75Vita/actions/runs/36822854734), del commit `c389f5e`, llegó a `Built target touhou75_vita`. Después falló con:

```text
vita-elf-create: Cannot allocate 1652 bytes for SCE data at end of segment 0; segment 1 overlaps
```

`Makefile:7: all Error 2` es la consecuencia. El convertidor necesita añadir metadatos de módulo/imports al segmento de código, pero el segmento siguiente estaba demasiado cerca. Se añade al enlace `--defsym=__sce_headroom=0x10000`: el [script de enlace de VitaSDK](https://github.com/vitasdk/buildscripts/blob/master/patches/binutils/0001-vita.patch) usa ese símbolo para reservar 64 KiB antes del siguiente segmento y mantener la alineación de Vita. La reserva es espacio entre segmentos, no un buffer del juego ni memoria x86 invitada.

Actions también muestra los encabezados de programa del ELF enlazado aunque falle la conversión, para poder revisar la separación real. No se compiló localmente porque aquí no está instalado VitaSDK. Posteriormente el usuario pudo instalar y ejecutar la VPK de Iteración 10 y aportó los resultados satisfactorios de hardware.

## Alcance

Se crea un entorno de diagnóstico para **un hilo**: TEB con NT_TIB, límites de pila, puntero a sí mismo, LastError, puntero a PEB y 64 slots TLS. El PEB contiene la base del EXE, el handle del heap y una estructura mínima de parámetros con cadenas ANSI/UTF-16 y ambiente vacío. No es una implementación completa del entorno Windows y no permite aún arrancar Touhou.

El runtime WinVita proporciona `set_fs_base()` y `guest_thread_ctx` para localizar el TEB. Los shims de errores comparten ahora TEB+0x34; se elimina el estado LastError independiente en el host. Los [offsets x86 documentados por Microsoft](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/debugging-a-stack-overflow) guían NT_TIB/TEB. Los detalles internos pueden cambiar entre versiones de Windows.

Se añaden siete imports de diagnóstico:

- TlsAlloc, TlsSetValue, TlsGetValue y TlsFree: índices 0..63 en el hilo principal, inicialización a cero, valores distintos por slot, índice fuera de rango, liberación, doble liberación rechazada y reutilización sin valor residual. No se implementan slots de expansión ni otros hilos.
- GetCommandLineA: devuelve una cadena invitada estable, `"C:\TH075\TH075.exe"`.
- GetModuleHandleA: solo el caso NULL devuelve la base del EXE. Los módulos por nombre siguen pendientes.
- GetStartupInfoA: escribe una estructura de 68 bytes con cb correcto y resto a cero, sin handles de consola heredados.

TlsGetValue devuelve cero y limpia LastError cuando accede con éxito a un índice admitido, de acuerdo con su [contrato](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-tlsgetvalue). La [inicialización a cero](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-tlsalloc) se comprueba después de asignar y reutilizar índices. Estos shims son contratos parciales declarados, no compatibilidad completa de KERNEL32.

## Pruebas x86

1. Repetir SHA-256, carga PE, control CPU, tres llamadas IAT, heap y archivo.
2. Ejecutar MOV con prefijo FS para leer Self, StackBase, StackLimit, PEB y LastError. Este último debe reflejar el error 6 de la prueba de segundo cierre de archivo.
3. SetLastError por la IAT -> lectura FS del mismo valor. Escritura FS de LastError -> GetLastError por la IAT devuelve ese valor.
4. Reservar dos índices TLS, comprobar ceros, escribir valores distintos, leer uno directamente por FS y ambos por APIs. Rechazar índice 64 y doble liberación; reutilizar un índice sin datos residuales. Al final deben quedar cero slots ocupados.
5. Comprobar que GetModuleHandleA(NULL) coincide con PEB.ImageBaseAddress, que la cadena de GetCommandLineA se puede leer desde el invitado y que GetStartupInfoA escribe cb=68.

Las llamadas comprueban IAT, bytes leídos de vuelta, EIP sentinel y ESP restaurado. Se descarta el bloque traducido antes de sustituir la rutina de pruebas. Las ocho líneas de la pantalla muestran SHA256, CPU, IAT, HEAP, FILE READ, TEB FS, TLS y PROCESS.

## Memoria añadida

- Código de pruebas: `0x00713000`, una página.
- TEB: `0x00730000`, una página; TLS dinámico en offset `0xE10`.
- PEB: `0x00731000`, una página.
- Cadenas/parámetros: `0x00732000`, una página.

Estas regiones caben en los 16 MiB invitados y no se superponen con las rutinas, buffers, pila, trampas ni heap de la Iteración 09. Se esperan **17 imports con shims de diagnóstico y 140 pendientes**.

## Cómo probar

1. Commit y push de esta revisión; esperar el workflow exitoso del commit nuevo.
2. Descargar **Touhou75Vita-iteration10-teb-fs-tls-vpk**, extraer e instalar la VPK. Mantiene T075VITA1, versión de paquete `01.12`.
3. Ejecutar **Touhou 7.5 Vita - Iteration 10 Thread Context Test** con el mismo EXE japonés. No necesitas añadir archivos.
4. Fotografiar la pantalla, pulsar X y compartir `iteration10.log` y `iteration10-runtime.log`.

Resultado confirmado en los logs de hardware:

```text
build_id=iteration10-winvita-teb-fs-tls-r1
game_code_executed=no
thread_context_result=initialized
import_bridge_unresolved=140
import_smoke_result=passed
heap_smoke_result=passed
file_smoke_result=passed
teb_fs_smoke_result=passed
tls_live_slots=0
tls_smoke_result=passed
process_smoke_result=passed
result=identity_services_teb_tls_process_passed
screen_result=presented
screen_active_matches=yes
```

## Lo que falta para el arranque

La cadena de excepciones está vacía; no se gestionan SEH/unwind, TLS estático del PE ni callbacks. Los IDs 4/8 solo identifican el proceso/hilo de diagnóstico; no hay scheduler ni CreateThread. PEB.Ldr, búsqueda de DLLs, entorno completo, locale, sincronización y la mayoría de APIs siguen pendientes. Direct3D 8, DirectInput y audio también. El entry point continúa sin invocarse. Después de validar FS/TLS se debe analizar el inicio real del ejecutable y preparar la prueba de arranque con parada explícita ante el primer servicio no implementado; no ejecutar el EXE con shims que fabrican éxito.
