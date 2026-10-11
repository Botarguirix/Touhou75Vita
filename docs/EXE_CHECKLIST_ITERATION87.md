# Comprobaciones — iteración 87

## Confirmado hasta 86

- Menú original sin opening; navegación física y sonidos limpios confirmados.
- 272 Present, 5811 dibujos; selección original 0..9, 250 esperas de TitleScene.
- Stop por 180 s; memoria gráfica 29,0 MiB final/pico.

## Comprobar en 87

| Área | Evidencia |
|---|---|
| Identidad | 87/01.91, iteration87-parallel-raster-r1. |
| Workers | Afinidad/núcleo 1/2 verificados, dibujos paralelos reales, fallback con motivo si falla. |
| Píxeles | Captura del menú consistente; filtros, alpha, capas y textos correctos. |
| Entrada | Navegación/confirmación/cancelación sin regresión y nueva escena o frontera registrada. |
| Rendimiento | Draw/Present/frame time y frames por tiempo; no confundir mayor actividad multicore con lentitud. |
| Diagnóstico | Hashes disabled explícitos; caché compara todos los bytes, scanout confirmado. |
| Audio | Efectos/voz original limpios al dibujar en paralelo; no se recupera el opening omitido. |
| Recursos | Sin incremento inesperado, jobs unidos antes de mutaciones y todos los workers joined/deleted. |
| Fronteras | Draw/pixel/time limits correctos; APIs desconocidas se detienen. |

106 grupos locales/compilación/paquete comprobados; rendimiento 87 físico pendiente.
[Detalle](STATUS_ITERATION87.md), [ruta GPU](PERFORMANCE_ITERATION87.md).
