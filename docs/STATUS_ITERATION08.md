# Iteración 08 — llamadas x86 por la IAT hacia Vita

Estado: r1 falló la prueba LastError; r2 pasó en la PS Vita del usuario. El log identifica `iteration08-winvita-iat-bridge-r2` y termina con `result=pe_mapped_dynarec_and_iat_smoke_passed`. Esto confirma la prueba limitada del puente, no el arranque del juego.

## Resultado r2 en consola

Los dos logs proporcionados se revisaron el 2026-09-30. El reporte confirma los enlaces IAT de las tres APIs, SetLastError con argumento `0x775`, GetLastError con retorno `0x775`, ESP `0x009FF000` y EIP sentinel `0x00BFFFF0` en ambas rutinas. GetTickCount devuelve `0x1751` y pasa la comprobación de rango del reloj nativo. Los contadores son `set:1 get:1 tick:1`; no se ejecutó código del juego y quedan 154 imports pendientes. Se conserva un extracto en `docs/hardware/iteration08-r2/result-excerpt.txt`.

La corrección elimina el fallo observado de r1. El resultado es consistente con la hipótesis de traducción antigua reutilizada; esta prueba no separa el efecto de cambiar las direcciones del de descartar la caché. El log de progreso del motor contiene dos grupos de inicialización porque se acumula entre lanzamientos; el reporte principal identifica la ejecución r2.

## Próximo hito

Preparar la Iteración 09 para comprobar identidad SHA-256 en Vita y llamadas x86 de reserva/liberación de memoria y lectura de archivos mediante APIs Win32. Auditar y reutilizar los mecanismos genéricos del runtime cuando sus contratos encajen con Touhou. Acompañar los resultados con una pantalla de diagnóstico persistente; esa pantalla prueba la aplicación Vita y no el renderer del juego. Después hacen falta el entorno TEB/FS/TLS y una prueba controlada del arranque original que identifique la primera API o instrucción bloqueante. La Iteración 09 está propuesta, todavía no implementada.

## Resultado r1 y corrección r2

Ambos logs aportados por el usuario se leyeron en esta sesión. Se conserva un extracto de resultados en `docs/hardware/iteration08-r1/result-excerpt.txt`; la unidad D: dejó de estar disponible al intentar copiar los archivos completos. El reporte contiene `dynarec_smoke_result=passed`, `import_bridge_unresolved=154`, EAX `0`, ESP `0x009FF000`, `import_smoke_calls=set:0 get:1 tick:0` y `result=failed`. GetTickCount no llegó a probarse. El progreso del motor registra la arena y el primer segmento JIT; no identifica un error de asignación.

La rutina CPU inicial y el test de imports reutilizaban `0x00700000`. `CpuBox86::map()` actualiza las protecciones sin destruir traducciones; `invalidate_code()` solo actúa si la región sigue marcada como protegida por el dynarec. Después de volver a mapear la página, esa condición puede desaparecer. Esto permite reutilizar el bloque anterior `mov eax,0x75; jmp 0x00B00000`; con el nuevo handler, `0x00B00000` corresponde al primer shim, GetLastError. Ese recorrido explica los contadores y ESP del log, pero es una hipótesis a confirmar con r2.

r2 coloca las tres rutinas en páginas diferentes y usa `discard_code()` antes de escribir cada prueba de imports. Añade comprobación de los bytes leídos de vuelta, enlaces IAT y logs de entrada de cada shim, argumento de SetLastError y EIP final. No modifica las semánticas ni rebaja los criterios de éxito.

## Objetivo

Tras el éxito limitado de la Iteración 07, comprobar el puente WinVita usando las direcciones IAT reales del EXE japonés. Las rutinas ejecutadas siguen siendo sintéticas; no se llama al entry point, callbacks TLS ni código del juego. El parche inglés sigue sin cargarse.

Se registran tres shims de diagnóstico: `SetLastError` recibe un argumento por la pila x86; `GetLastError` devuelve el valor guardado; `GetTickCount` devuelve milisegundos desde el inicio del proceso Vita. El estado LastError pertenece únicamente a esta prueba de un hilo y el origen del reloj no representa el arranque de Windows. Estos shims todavía no constituyen el entorno Win32 del juego.

Cada llamada usa `call dword ptr [IAT]` y termina con `ret` al sentinel del Bridge. Se comprueban el retorno, la restauración exacta de ESP, los contadores de llamadas y que el reloj devuelto esté entre dos lecturas nativas. Los demás imports se enumeran como `unsupported`; si se invocan accidentalmente, se registra la API, se solicita detener la prueba al salir del shim y se marca fallo.

## Memoria y logs

- Imagen japonesa: base `0x00400000`, final inferior a `0x00700000`, comprobado antes del mapeo.
- Prueba CPU básica: `0x00700000`. Prueba LastError: `0x00710000`. Prueba GetTickCount: `0x00711000`; una página por rutina.
- Pila del Bridge: `0x00800000..0x00A00000`.
- Trampas: `0x00B00000..0x00C00000`. La primera prueba CPU inicializa esta misma ventana completa porque CpuBox86 solo genera los stubs en la primera instalación de trampas.
- Todo permanece dentro del espacio invitado de 16 MiB de la prueba anterior.
- Reporte: `ux0:data/TH075Vita/iteration08.log`, sin buffering.
- Progreso del motor: `ux0:data/TH075Vita/iteration08-runtime.log`, separado para evitar que dos archivos abiertos sobre el mismo log sobrescriban sus líneas. El progreso del motor puede contener ejecuciones anteriores; identificar la prueba por el reporte principal.

## Prueba en consola

1. Subir los cambios y esperar un workflow exitoso del commit nuevo.
2. Descargar `Touhou75Vita-iteration08-r2-iat-bridge-vpk`, extraer el ZIP e instalar la VPK con VitaShell.
3. Mantener el EXE japonés en `ux0:data/TH075Vita/TH075.exe` y abrir la aplicación **Iteration 08 r2**, versión de paquete `01.09`. Sigue siendo la corrección r2 de la Iteración 08.
4. Compartir `iteration08.log`; si falla o queda incompleto, compartir también `iteration08-runtime.log`.

Resultado esperado:

```text
build_id=iteration08-winvita-iat-bridge-r2
game_code_executed=no
dynarec_smoke_result=passed
import_bridge_unresolved=154
import_smoke_lasterror_eax=0x00000775
import_smoke_lasterror_result=passed
import_smoke_tickcount_result=passed
import_smoke_tickcount_range=passed
import_smoke_calls=set:1 get:1 tick:1
import_smoke_result=passed
result=pe_mapped_dynarec_and_iat_smoke_passed
```

Los 154 imports pendientes son esperados para esta prueba; no indican compatibilidad suficiente para arrancar. Después: verificar hash en Vita, preparar TEB/FS/TLS, heap y archivos, auditar los shims genéricos del motor y añadir las APIs requeridas por el inicio del juego. Direct3D 8, DirectInput, WinMM y COM requieren trabajo adicional. El objetivo siguiente de arranque debe detenerse explícitamente ante APIs no implementadas, sin fabricar éxito.
