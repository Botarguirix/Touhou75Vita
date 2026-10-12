# Roadmap actual — selección visible en la 95

La selección ya se presenta en **Practice y Story**. Las dos capturas terminan
en GetTimeZoneInformation. La traza disponible registra 188 Present, 339 draws
de área cero aceptados y 73,81 MiB gráficos, sin rechazo de memoria/geometría.
El siguiente paso es atender la consulta de zona horaria y continuar la carga.

Estamos más cerca del primer combate. Selección mide 135,287 ms por ciclo
(7,39/s); el objetivo de 60 FPS exige unos 16,667 ms, además de lógica e input
correctos. Combate y rendimiento final todavía requieren evidencia física.

| Orden | Hito | Resultado exigido |
|---|---|---|
| 1 | GetTimeZoneInformation, propuesta 96 | Contrato x86/RTC probado y confirmación de personaje supera esa API |
| 2 | Carga de Practice y Story | Logs separados registran la siguiente escena y sus recursos |
| 3 | Combate visible | Arena, dos personajes y HUD en varios frames sucesivos |
| 4 | Combate controlable | Movimiento, salto, ataques, guardia, dash y pausa comprobados |
| 5 | Renderer GXM | Selección GPU comparada con CPU, con menor tiempo medido |
| 6 | Texturas/Present | Upload por cambio y presentación directa, con coherencia y ownership |
| 7 | Guest y carga | Hot paths, imports, DAT y trabajo auxiliar optimizados con regresiones |
| 8 | Cadencia y audio | 60 updates/frames por segundo, controles y sonido estables |
| 9 | Rondas y modos | Reinicios/cambios y avance Story sin fugas ni nuevas fronteras |
| 10 | Aceptación y calidad | Combate a 60 FPS durante 10 minutos y recuperación gradual de resolución |

[Roadmap detallado, evidencias y condiciones de cierre](ROADMAP_AFTER_SELECTION95.md)
· [Recursos de reconstrucción](PLAYABLE_PATH.md)
· [Mediciones de rendimiento](PERFORMANCE_60FPS.md).

Se conserva el menú directo, 320×240 provisional, raster serial, audio/input
nativos y arte DAT de Alice. El trabajo GPU puede comenzar con selección
como referencia mientras se completa combate. No hay una estimación fiable
de cuántas iteraciones faltan. El VPK 96 aún no se ha generado.

## Historial

Los planes y evidencias anteriores quedan en los reportes
[92](STATUS_ITERATION92.md), [93](STATUS_ITERATION93.md),
[94](STATUS_ITERATION94.md) y [95](STATUS_ITERATION95.md).
