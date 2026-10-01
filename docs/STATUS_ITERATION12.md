# Iteración 12 — servicios iniciales del EXE original

**Estado:** código preparado localmente; pendiente compilar en GitHub Actions y ejecutar en la misma Vita. No hay VitaSDK instalado en este PC. El resultado de esta iteración todavía no está confirmado.

## Punto de partida confirmado

Los logs de Iteración 11 R2 y la foto prueban que el EXE japonés ejecutó instrucciones originales desde `0x0064232C` hasta `KERNEL32.dll!GetVersionExA`, con pila y registro SEH válidos, sin timeout. Véase la [evidencia de hardware](hardware/iteration11-r2/result-excerpt.txt). El traductor x86 a ARMv7 y el cargador PE ya están integrados mediante WinVita; VitaSDK compila la aplicación nativa. No hace falta otra biblioteca para el tramo de esta iteración.

## Qué cambia

El EXE se restaura desde la copia verificada por SHA-256 y se preparan un TEB/PEB y pila nuevos después del preflight. Se mantienen las ocho comprobaciones previas, los 157 interceptores IAT, el presupuesto aproximado de 512 entradas a bloques y el watchdog nativo de cinco segundos validado en R2.

`startup_services.cpp` implementa únicamente las variantes y direcciones de llamada conocidas de este EXE:

| API | Respuesta / comprobación |
| --- | --- |
| `GetVersionExA` | Valida tamaño ANSI de 148 bytes y buffer en la pila; inicializa todos los bytes y devuelve perfil de compatibilidad XP 5.1, build 2600, plataforma NT 2, sin service pack. Retorna BOOL TRUE y limpia el argumento según stdcall. |
| `GetModuleHandleA(NULL)` | Devuelve la base real del EXE cargado, `0x00400000`, tras comprobar sus cabeceras DOS y PE en memoria invitada. Limpia el argumento según stdcall. |
| `HeapCreate` | Se detiene antes de servirla. Comprueba IAT `0x00657160`, retorno `0x0064974C` y argumentos `(0, 4096, 0)`. |

El perfil XP es una decisión de compatibilidad, no la detección del sistema operativo de la consola. Microsoft documenta el buffer y valor de retorno de [GetVersionExA](https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getversionexa), los campos de [OSVERSIONINFOA](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-osversioninfoa) y que [GetModuleHandleA(NULL)](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandlea) devuelve el módulo del EXE del proceso. No se anuncia compatibilidad completa con esas APIs: estructuras extendidas, llamadas desde otras direcciones y módulos por nombre quedan fuera del contrato admitido en este checkpoint.

Cada retorno verifica EAX, EIP, limpieza de ESP y conservación de EBX, EBP, ESI y EDI. El código original debe consumir la estructura de versión y llenar sus cinco globals con los valores previstos. Se comprueba esa memoria al llegar a `HeapCreate`; el código nativo no escribe esos globals.

Un import desconocido se registra y detiene la ejecución. Las instrucciones originales del EXE se conservan. No se habilitan todos los shims parciales del runtime ni se devuelve éxito para una API desconocida. El contador de 17 shims parciales / 140 imports pendientes del preflight corresponde a los diagnósticos anteriores; los dos contratos nuevos pertenecen a esta fase separada del arranque.

## Compilar y probar en una Vita

1. En GitHub Desktop, abrir **Touhou75Vita** en la rama `codex/d2vita-runtime-review`. Revisar los cambios y usar el mensaje de commit `feat: continue TH075 startup through version and module services`.
2. Pulsar **Commit**, después **Push origin**. Abrir [Actions](https://github.com/Botarguirix/Touhou75Vita/actions) y entrar en la ejecución correspondiente a ese commit. Esperar el resultado verde.
3. En **Artifacts**, descargar **Touhou75Vita-iteration12-startup-services-vpk**, extraer el ZIP e instalar su VPK con VitaShell. Si aparece un error de compilación, compartir la primera línea de error del paso Build, además del resumen final de make.
4. Abrir **Touhou 7.5 Vita - Iteration 12 Startup Services**, versión `01.15`, con el mismo EXE japonés en `ux0:data/TH075Vita/TH075.exe`. La pantalla debe decir **ITERATION 12**. Mantiene el título `T075VITA1`.
5. Fotografiar la pantalla, pulsar X y compartir los tres archivos de esta ejecución en `ux0:data/TH075Vita/`: `iteration12.log`, `iteration12-runtime.log` e `iteration12-watchdog.log`. Si la app se cierra antes de presentar resultados, compartir los archivos disponibles.

Resultado esperado, **todavía no observado**:

```text
build_id=iteration12-startup-version-module-r1
preflight_result=passed
game_entrypoint=attempted
startup_entry_checkpoint=passed
startup_first_import=KERNEL32.dll!GetVersionExA
startup_serviced_import=KERNEL32.dll!GetVersionExA
startup_version_buffer_readback=passed
startup_serviced_import=KERNEL32.dll!GetModuleHandleA
startup_module_base_returned=0x00400000
startup_module_headers=passed
startup_stop_import=KERNEL32.dll!HeapCreate
startup_return_va=0x0064974C
startup_heap_flags=0x00000000
startup_heap_initial_bytes=4096
startup_heap_maximum_bytes=0
startup_heap_call_frame=passed
startup_version_globals=passed
startup_version_calls=1
startup_module_calls=1
startup_serviced_imports=2
startup_stop_api_executed=no
startup_limit_hit=no
game_code_executed=yes
startup_result=reached_heap_create
result=real_entrypoint_version_module_passed
```

El orden de las líneas puede diferir; las claves repetidas muestran la progresión y se interpreta su última aparición al mostrar el resultado. Se espera `watchdog_revision=iteration12-r1` y `watchdog_result=disarmed` en el log separado. La pantalla debe mostrar **STARTUP CHECKPOINT PASS**, **EXE: TWO SERVICES RETURNED** y **STOP: HEAPCREATE**; el arranque completo continúa sin verificarse.

## Próximo requisito

Si este recorrido pasa en hardware, el siguiente trabajo es servir `HeapCreate` con un heap invitado real y continuar hasta la dependencia que revele el EXE. Las pruebas sintéticas de heap no validan todavía el heap requerido por el arranque del juego. Más adelante harán falta servicios de archivos, módulos, ventanas, D3D8, controles y audio según el recorrido real. El parche inglés se deja para después del arranque japonés; las otras Vitas también.
