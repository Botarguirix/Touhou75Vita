# Estado actual

**Fecha:** 2026-10-01

**Hito:** Iteración 11 R1 pasó el preflight pero no creó el watchdog; corrección R2 preparada localmente

## Confirmado

- La VPK de Iteración 10 se ejecutó en la Vita física y verificó por SHA-256 el EXE japonés en `ux0:data/TH075Vita/TH075.exe`.
- WinVita cargó y mapeó el PE32 x86 de 2,576,384 bytes, en su base preferida `0x00400000`. El entry point es `0x0064232C`.
- Pasaron las ocho comprobaciones sintéticas de identidad, CPU, IAT, heap, archivo, TEB/FS, TLS y proceso. La pantalla se confirmó con log e imagen y la app salió con X.
- Hay 157 imports; 17 tienen shims parciales de diagnóstico y 140 siguen sin implementar. Esa cobertura no equivale a compatibilidad completa de Windows.
- En el log validado de Iteración 10, `game_code_executed=no`: el EXE está cargado pero todavía no se invocó su entry point.
- Iteración 11 R1 repitió esas ocho comprobaciones y se detuvo con `0x80028023` al crear el watchdog con una prioridad inválida. El entry point todavía no se invocó.
- Iteración 11 R2 corrige la prioridad y registra la colocación de los hilos. Falta compilarla y ejecutarla en la misma Vita.

## Ejecutables en estudio

| Versión | Archivo | SHA-256 | Análisis |
|---|---|---|---|
| Japonesa | `TH075.exe` | `BD441E99075436E8DCAD26F86FFCF5E6AAC4F58B0ED3EE7442E4CB39D8E22C98` | Coincide con el EXE incluido en el paquete de análisis; hay una lista de 157 imports estáticos. |
| Parche inglés | `TH075E.exe` | `C8313228A98134B5CEB75027D77B302E4271670B17F6B52593A1D41679834D0E` | Según el usuario, complementa el juego japonés. Log de Vita: PE32 I386, 9.728 bytes, 6 secciones. No se ejecutó. |

El juego base que se debe portar es el japonés `TH075.exe`. Su hash ya fue verificado en la consola. El parche inglés no reemplaza al juego; su integración se deja para después del arranque de la versión japonesa.

## Pendiente inmediato

1. Commit y push de Iteración 11 R2; generar la VPK con Actions.
2. Probar en **una sola Vita**, manteniendo el mismo EXE japonés.
3. Confirmar si el recorrido real llega a `KERNEL32.dll!GetVersionExA` con pila y registro FS correctos. Recoger `iteration11.log`, `iteration11-runtime.log` y `iteration11-watchdog.log`.
4. Trabajar sobre la primera dependencia real o sobre el fallo de CPU que indique el log; continuar el inicio sin fabricar éxito en servicios desconocidos.
5. Implementar los servicios posteriores, gráficos D3D8, entrada y audio según el recorrido observado. Las otras tres Vitas se usarán cuando el EXE ya arranque.

Los resultados están en [STATUS_ITERATION10.md](STATUS_ITERATION10.md). El procedimiento nuevo está en [STATUS_ITERATION11.md](STATUS_ITERATION11.md). El proyecto todavía no tiene un arranque completo del juego confirmado en hardware.
