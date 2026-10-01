# Iteración 11 — primera invocación del EXE original

Estado: código preparado localmente; **compilación en Actions y ejecución en Vita pendientes**. No hay VitaSDK instalado en este PC. La Iteración 10 ya pasó las ocho pruebas en la Vita física del usuario. Esta revisión hace la primera llamada al punto de entrada original; un resultado satisfactorio confirma solo el recorrido hasta la primera API, sin establecer aún que el juego pueda arrancar completo.

## Qué ejecuta

Después de repetir las comprobaciones de Iteración 10, se restaura la imagen PE original verificada por SHA-256, se descartan sus traducciones anteriores y se prepara una pila y un TEB/PEB nuevos en el mismo runtime. Los registros generales empiezan en cero y EFLAGS en `0x202`. El EXE conserva sus instrucciones; las 157 entradas IAT se dirigen a interceptores que detienen la CPU antes de servir cualquier API.

Se invoca `0x0064232C`. Se espera observar el registro de excepciones por `FS:[0]`, el ajuste de pila y la llamada original a `KERNEL32.dll!GetVersionExA` por IAT `0x00657090`. El checkpoint exige:

- Dirección de retorno `0x00642352`.
- Argumento dentro de la pila invitada y buffer de 148 bytes con su campo de tamaño igual a `148`.
- Registro de excepciones dentro de la pila, predecessor `0xFFFFFFFF` y handler `0x00645468`.
- Parada limpia sin consumir el presupuesto de ejecución.

El interceptor conserva la pila y los registros al llegar a la API: no escribe un resultado ni ejecuta una instrucción posterior a la llamada. El registro de una estructura SEH no implica que se puedan despachar excepciones. Se rechaza el inicio si el PE necesita un directorio TLS estático; el EXE japonés analizado tiene ese directorio vacío.

## Límites y diagnóstico

El backend WinVita interpreta `set_run_limit(4096)` como unas 512 **entradas a bloques**, no como exactamente 4096 instrucciones. Una vuelta dentro del mismo bloque puede eludir esa cuenta. Por eso un hilo nativo independiente, con afinidad USER_1 y prioridad superior, supervisa un plazo de cinco segundos para este tramo. Se usan las [APIs de hilos de VitaSDK](https://github.com/vitasdk/vita-headers/blob/master/include/psp2/kernel/threadmgr/thread.h).

Si no se puede iniciar el watchdog, no se invoca el EXE. Si se agota el presupuesto o aparece un error informado por la CPU, la app vuelve a la pantalla de resultados. Si vence el watchdog, escribe su log separado y llama a `sceKernelExitProcess(124)`; no se intenta desenrollar un dynarec bloqueado. Un fallo nativo que termine el proceso puede impedir que aparezca la pantalla; los marcadores previos a la llamada se escriben sin buffering para conservar el último estado disponible.

Archivos en `ux0:data/TH075Vita/`:

- `iteration11.log`: preflight, punto de entrada, primer import, pila/SEH, registros, resultado y pantalla.
- `iteration11-runtime.log`: progreso del runtime/JIT.
- `iteration11-watchdog.log`: preparación, desarme normal o timeout y EIP muestreado cuando esté disponible. Este archivo se crea justo antes de la prueba real; si falla el preflight, no habrá un watchdog nuevo. La app intenta borrar el archivo anterior al comenzar y registra si no puede hacerlo.

## Probar en una sola Vita

Las otras tres consolas se dejan para después de que el EXE arranque, conforme a la indicación del usuario. No hace falta cambiar el juego, instalar otro runtime ni añadir assets para este checkpoint.

1. Commit y push de Iteración 11; esperar la ejecución nueva de Actions en verde.
2. Descargar **Touhou75Vita-iteration11-exe-startup-vpk**, extraer e instalar la VPK en la misma Vita usada antes. Mantiene el título `T075VITA1`; versión `01.13`.
3. Abrir **Touhou 7.5 Vita - Iteration 11 EXE Startup**, con el EXE japonés de siempre en `ux0:data/TH075Vita/TH075.exe`.
4. Fotografiar la pantalla y pulsar X. Compartir los tres logs nuevos. Si la aplicación se cierra antes de la pantalla, compartir los logs disponibles.

Resultado esperado, todavía **sin confirmar en hardware**:

```text
build_id=iteration11-original-entrypoint-first-import-r1
preflight_result=passed
startup_iat_intercept_count=157
game_entrypoint=attempted
startup_first_import=KERNEL32.dll!GetVersionExA
startup_return_va=0x00642352
startup_version_info_size=148
startup_seh_registration=passed
startup_api_executed=no
startup_limit_hit=no
game_code_executed=yes
startup_result=reached_first_import
result=real_entrypoint_first_import_passed
```

La pantalla distingue `ENTRY CHECKPOINT PASS` de `GAME BOOT: NOT YET VERIFIED`. El watchdog debe terminar con `watchdog_result=disarmed`. Un PASS de las ocho pruebas de preflight por sí solo no valida la ejecución original.

## Después del checkpoint

Si el recorrido se confirma, se implementará el contrato de `GetVersionExA` requerido por este EXE y se continuará hasta la siguiente dependencia real del inicio. Si falla antes, la dirección y el estado de pila/FS permitirán trabajar sobre el problema concreto del traductor. Persisten 140 imports sin servicios compatibles en el diagnóstico, además de gráficos D3D8, entrada, audio y arranque completo. No se estima aún una cantidad de iteraciones para obtener el menú.
