# Estado actual

**Fecha:** 2026-10-01

**Hito:** Iteración 11 R2 ejecutó el EXE original hasta su primera API en Vita; Iteración 12 preparada localmente

## Confirmado

- La VPK de Iteración 10 se ejecutó en la Vita física y verificó por SHA-256 el EXE japonés en `ux0:data/TH075Vita/TH075.exe`.
- WinVita cargó y mapeó el PE32 x86 de 2,576,384 bytes, en su base preferida `0x00400000`. El entry point es `0x0064232C`.
- Pasaron las ocho comprobaciones sintéticas de identidad, CPU, IAT, heap, archivo, TEB/FS, TLS y proceso. La pantalla se confirmó con log e imagen y la app salió con X.
- Hay 157 imports; 17 tienen shims parciales de diagnóstico y 140 siguen sin implementar. Esa cobertura no equivale a compatibilidad completa de Windows.
- Iteración 11 R1 se detuvo con `0x80028023` al crear el watchdog con una prioridad inválida; R2 corrigió ese fallo.
- Iteración 11 R2 confirmó `game_code_executed=yes` y `startup_result=reached_first_import`: instrucciones originales desde `0x0064232C` hasta `KERNEL32.dll!GetVersionExA`, con buffer de 148 bytes y registro SEH correctos.
- El watchdog se armó y desarmó, el presupuesto no se agotó y la pantalla se presentó. El log de esta ejecución no incluye todavía la salida con X.
- El arranque completo del juego, D3D8, controles y audio siguen sin verificarse.

## Ejecutables en estudio

| Versión | Archivo | SHA-256 | Análisis |
|---|---|---|---|
| Japonesa | `TH075.exe` | `BD441E99075436E8DCAD26F86FFCF5E6AAC4F58B0ED3EE7442E4CB39D8E22C98` | Coincide con el EXE incluido en el paquete de análisis; hay una lista de 157 imports estáticos. |
| Parche inglés | `TH075E.exe` | `C8313228A98134B5CEB75027D77B302E4271670B17F6B52593A1D41679834D0E` | Según el usuario, complementa el juego japonés. Log de Vita: PE32 I386, 9.728 bytes, 6 secciones. No se ejecutó. |

El juego base que se debe portar es el japonés `TH075.exe`. Su hash ya fue verificado en la consola. El parche inglés no reemplaza al juego; su integración se deja para después del arranque de la versión japonesa.

## Pendiente inmediato

1. Commit y push de Iteración 12; generar la VPK con Actions. No se compiló localmente porque este PC no tiene VitaSDK.
2. Probar en **una sola Vita**, manteniendo el mismo EXE japonés.
3. Confirmar que el recorrido vuelve de `GetVersionExA` y `GetModuleHandleA(NULL)`, consume sus resultados y llega a `HeapCreate(0, 4096, 0)`. Recoger `iteration12.log`, `iteration12-runtime.log` y `iteration12-watchdog.log`.
4. Si pasa, implementar el heap requerido por el juego y continuar hasta su siguiente dependencia. Si falla, trabajar sobre la dirección y contrato que indique el log.
5. Implementar los servicios posteriores, gráficos D3D8, entrada y audio según el recorrido observado. Las otras tres Vitas se usarán cuando el EXE ya arranque.

Los resultados originales están en [STATUS_ITERATION11.md](STATUS_ITERATION11.md). El procedimiento nuevo está en [STATUS_ITERATION12.md](STATUS_ITERATION12.md). Ya están integrados el cargador PE y el traductor x86 a ARMv7 de WinVita; el requisito inmediato son los contratos Win32 que reclama el arranque. El proyecto todavía no tiene un arranque completo del juego confirmado en hardware.
