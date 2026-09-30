# Estado actual

**Fecha:** 2026-09-30  
**Hito:** Iteración 05 probado en Vita; fuente de Iteración 06 preparada localmente

## Confirmado

- GitHub Actions generó el VPK de diagnóstico.
- La Vita ejecutó la aplicación y leyó `ux0:data/TH075Vita/TH075.exe`.
- El log reconoce un PE32 x86 (I386) de 2,576,384 bytes, con cinco secciones y base preferida `0x00400000`.
- El log dice `execution=not_attempted`; el juego no se ha cargado ni ejecutado.
- El paquete de ingeniería inversa incluye una lista de 157 imports estáticos.
- WinVita contempla imágenes PE sin relocaciones y las mantiene en su base preferida. Falta probar esa ruta con el EXE de Touhou.

## Ejecutables en estudio

| Versión | Archivo | SHA-256 | Análisis |
|---|---|---|---|
| Japonesa | `TH075.exe` | `BD441E99075436E8DCAD26F86FFCF5E6AAC4F58B0ED3EE7442E4CB39D8E22C98` | Coincide con el EXE incluido en el paquete de análisis; hay una lista de 157 imports estáticos. |
| Parche inglés | `TH075E.exe` | `C8313228A98134B5CEB75027D77B302E4271670B17F6B52593A1D41679834D0E` | Según el usuario, complementa el juego japonés. Log de Vita: PE32 I386, 9.728 bytes, 6 secciones. No se ejecutó. |

El juego base que se debe portar es el japonés `TH075.exe`. El parche inglés no reemplaza al juego: se deberá estudiar por separado para saber cómo aplica la traducción. El log de Vita confirma el tamaño y los campos PE del juego japonés, pero no calcula la huella de la copia instalada en la consola.

## Pendiente inmediato

1. Mantener el japonés `TH075.exe` como imagen principal del port.
2. Inspeccionar `TH075E.exe` como parche de traducción y determinar qué cambios de código/datos requiere; el log actual no demuestra que el parche se haya ejecutado.
3. Integrar WinVita en una rama de prueba con la revisión y avisos de licencia fijados.
4. Hacer una prueba del cargador con el juego japonés que mapee cabeceras y secciones en `0x00400000`, e informe imports sin resolver. Esa prueba debe detenerse antes del entry point.
5. Usar los resultados para decidir qué puente Win32 implementar primero. D3D8, DirectInput 8 y WinMM son subsistemas separados; el renderer Glide de D2Vita no resuelve D3D8.

La Iteración 06 aún no se ha compilado ni publicado: el VPK anterior solo revisa `TH075.exe`. El diagnóstico y la arquitectura están descritos en [STATUS_ITERATION05.md](STATUS_ITERATION05.md), [STATUS_ITERATION06.md](STATUS_ITERATION06.md) y [PORTING_ARCHITECTURE.md](PORTING_ARCHITECTURE.md). El proyecto todavía no tiene un VPK que arranque el juego.
