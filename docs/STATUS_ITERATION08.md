# Iteración 08 — llamadas x86 por la IAT hacia Vita

Estado: código preparado localmente. Compilación y prueba en consola pendientes. Este entorno no tiene VitaSDK instalado; no se afirma que la nueva VPK haya compilado.

## Objetivo

Tras el éxito limitado de la Iteración 07, comprobar el puente WinVita usando las direcciones IAT reales del EXE japonés. Las rutinas ejecutadas siguen siendo sintéticas; no se llama al entry point, callbacks TLS ni código del juego. El parche inglés sigue sin cargarse.

Se registran tres shims de diagnóstico: `SetLastError` recibe un argumento por la pila x86; `GetLastError` devuelve el valor guardado; `GetTickCount` devuelve milisegundos desde el inicio del proceso Vita. El estado LastError pertenece únicamente a esta prueba de un hilo y el origen del reloj no representa el arranque de Windows. Estos shims todavía no constituyen el entorno Win32 del juego.

Cada llamada usa `call dword ptr [IAT]` y termina con `ret` al sentinel del Bridge. Se comprueban el retorno, la restauración exacta de ESP, los contadores de llamadas y que el reloj devuelto esté entre dos lecturas nativas. Los demás imports se enumeran como `unsupported`; si se invocan accidentalmente, se registra la API, se solicita detener la prueba al salir del shim y se marca fallo.

## Memoria y logs

- Imagen japonesa: base `0x00400000`, final inferior a `0x00700000`, comprobado antes del mapeo.
- Rutinas sintéticas: `0x00700000`, una página.
- Pila del Bridge: `0x00800000..0x00A00000`.
- Trampas: `0x00B00000..0x00C00000`. La primera prueba CPU inicializa esta misma ventana completa porque CpuBox86 solo genera los stubs en la primera instalación de trampas.
- Todo permanece dentro del espacio invitado de 16 MiB de la prueba anterior.
- Reporte: `ux0:data/TH075Vita/iteration08.log`, sin buffering.
- Progreso del motor: `ux0:data/TH075Vita/iteration08-runtime.log`, separado para evitar que dos archivos abiertos sobre el mismo log sobrescriban sus líneas. El progreso del motor puede contener ejecuciones anteriores; identificar la prueba por el reporte principal.

## Prueba en consola

1. Subir los cambios y esperar un workflow exitoso del commit nuevo.
2. Descargar `Touhou75Vita-iteration08-iat-bridge-vpk`, extraer el ZIP e instalar la VPK con VitaShell.
3. Mantener el EXE japonés en `ux0:data/TH075Vita/TH075.exe` y abrir la aplicación versión `01.08`.
4. Compartir `iteration08.log`; si falla o queda incompleto, compartir también `iteration08-runtime.log`.

Resultado esperado:

```text
build_id=iteration08-winvita-iat-bridge-r1
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
