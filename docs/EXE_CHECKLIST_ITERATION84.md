# Comprobaciones — iteración 84

## Confirmado en Vita hasta 83

- SHA/PE32, entrypoint, IAT, heap, FS/TEB/TLS y servicios de arranque usados.
- Timer original, 230 contextos de espera main y 189 de despacho audio preservados.
- 228 Present; primeros 208 scanout y 397 hashes de dibujo coinciden con 82.
- 695 dibujos (465 LINEAR), memoria gráfica dentro de 64 MiB, escena nueva.
- PCM original audible más tiempo pero entrecortado; uso elevado de CPU reportado.
- Un Play y 44 uploads activos; salida/shutdown sin error. Causa de cortes pendiente.

## Comprobar ahora en Vita

| Área | Evidencia necesaria |
|---|---|
| Identidad | 84/01.88, build iteration84-triangle-strips-r1 y Alice original en LiveArea. |
| Dibujo nuevo | DrawPrimitiveUP 696 y siguientes ejecutados con convex_triangle_strip; sin consumo simulado. |
| Precisión | Bits de vértices completos; hashes previos conservados y comparación de nuevas capturas contra Windows. |
| Despacho | Contextos preservados; próxima frontera original, sin fault o llamada duplicada. |
| Audio nativo | output_timing: intervalos, bloqueos, espera de mutex, silencio; correlación con cortes oídos. |
| Worker/cursor | Lateness luego del primer despacho, lecturas/uploads originales y progreso de cursor. |
| Rendimiento | Tiempos del renderer por geometría y ciclos; distinguir ejecución de pantalla final. |
| Límites | 240 Present/130 s, watchdog 150 s; memoria, refs y cierre válidos. |

## Trabajo posterior

1. Completar perfiles originales de gráficos/servicios encontrados.
2. Medir el origen de los cortes; ajustar scheduler/cursor/salida sólo con evidencia.
3. Usar los costes medidos para diseñar aceleración GPU preservando los contratos.
4. Llegar al menú original y aceptar controles/selección real.
5. Combate, colisiones, audio continuo, round y retorno al menú.
6. Memoria/lifecycle/guardado sostenidos y rendimiento medido en Vita.

77 grupos portables, oráculo de 500 strips, digests -O0/-O2, ownership replay
y compilación/paquete comprobados. Esto no demuestra ejecución jugable.
Detalle: [status](STATUS_ITERATION84.md); fuentes: [investigación](RESEARCH_EXE_ITERATION73.md).
