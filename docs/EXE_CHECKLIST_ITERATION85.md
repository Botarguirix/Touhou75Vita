# Comprobaciones — iteración 85

## Confirmado en Vita hasta 84

- Arranque PE32/SHA/IAT, heap, FS/TEB/TLS y servicios usados.
- 239 Present, 757 dibujos; seis strips convexos originales nuevos ejecutados.
- 240 contextos main y 221 de despacho audio preservados; memoria dentro de 64 MiB.
- La parada fue agotar 240 reanudaciones, no una nueva API desconocida.
- Música original progresa; cortes coincidentes con carga CPU según el usuario.
- Entregas nativas regulares, pero 1048 de 2170 bloques silenciosos; causa completa pendiente.
- 53 Lock ENTIREBUFFER con offset no cero fueron colocados al inicio por el puente anterior.

## Comprobar ahora en Vita

| Área | Evidencia necesaria |
|---|---|
| Identidad | 85/01.89, iteration85-stream-offsets-r1 y Alice original en LiveArea. |
| PCM offsets | ptr1=staging+offset, spans cubren el anillo; ReadFile original y uploads en posiciones distintas. |
| Audio audible | Música progresa sin cortes/reinicios, o ubicación/duración de silencios y carga CPU. |
| Salida/worker | silent_blocks, output_timing, lateness tras primer wake; cursores y cola progresan. |
| EXE | Más de 239 Present, nueva escena/servicio o motivo preciso de parada acotada. |
| Gráficos | Geometría/bits/hashes comparados usando las mismas entradas; no exigir hashes de animaciones diferentes. |
| Rendimiento | Coste por dibujo/geometría, ciclos y salida; distinguir pantalla final del EXE. |
| Contratos | Contextos, errores, memoria/refs, cierre de audio/display y watchdog válidos. |
| Límites | 360 Present / 361 esperas / 180 s, watchdog 210 s; 6144 draws / 768 Mi píxeles / 64 MiB gráficos. |

## Trabajo posterior

1. Implementar fielmente la siguiente API/geometría que alcance el EXE.
2. Correlacionar muestras/silencios/cursor/refill con audio físico después de corregir offsets.
3. Diseñar aceleración GPU usando los costes medidos y contratos D3D8 observados.
4. Llegar al menú original, aceptar controles y selección real.
5. Combate, colisiones, audio, fin de round y retorno al menú.
6. Rendimiento/memoria/guardado sostenidos en Vita; comparar gráficos con Windows.

80 grupos portables, regresión con código anterior, oráculo, replay, compilación
y paquete comprobados; ejecución jugable aún pendiente.
[Detalle y paquete](STATUS_ITERATION85.md); [fuentes previas](RESEARCH_EXE_ITERATION73.md).
