# Comprobaciones — iteración 83

## Confirmado físicamente hasta 82

- SHA/PE32, entrypoint, IAT, heap, FS/TEB/TLS y servicios de arranque usados.
- Timer original, señales consumidas y 209 contextos main preservados.
- 208 Present reales; los primeros 203 hashes coinciden con 81.
- 397 dibujos (167 LINEAR) y recursos dentro de 64 MiB.
- PCM original audible, output/drain sin error, dos reposiciones del stream.
- Instalación 82 confirmada; música repetitiva y captura final negra.

## Comprobar ahora en Vita

| Área | Evidencia necesaria |
|---|---|
| Identidad | 83/01.87, build iteration83-audio-scheduling-r1; portada Alice original. |
| LINEAR más rápido | Tiempos draw/frame y contadores constant/exact_dyadic/reference_double; hashes conservados. |
| Despacho entre llamadas | audio_trap_yield y context=preserved; no CPU fault, corrupción de callframe o consumo duplicado. |
| Reposición original | El worker 13 consulta cursores y lee nuevos bytes con frecuencia suficiente; no se limita a consultar status. |
| Música | El usuario confirma continuidad; duración y discontinuidades anotadas, sin confundir bloques enviados con audibilidad. |
| Avance gráfico | Present posteriores a 208, contenido visible y próxima frontera del opening. |
| Recursos | Memoria/refcounts y shutdown/port release correctos, watchdog desarmado. |

## Trabajo que sigue pendiente

1. Completar los perfiles D3D8 originales que aparezcan y comparar contra Windows.
2. Scheduler sostenido de todos los workers y precisión temporal/cursor PCM.
3. Alcanzar el menú del EXE, entrada real y selección de modo/personaje.
4. Combate, colisiones, controles, audio sostenido, round y retorno al menú.
5. Guardado, lifecycle prolongado, memoria y rendimiento medidos en hardware.

66 grupos portables, digests -O0/-O2, replay ownership y compilación aprobados.
El runner nuevo y la continuidad del audio requieren su corrida física.
Detalle: [status](STATUS_ITERATION83.md); fuentes: [investigación](RESEARCH_EXE_ITERATION73.md).
