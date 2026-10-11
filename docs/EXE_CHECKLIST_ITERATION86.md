# Comprobaciones — iteración 86

## Confirmado en Vita hasta 85

- Original japonés: 339 Present, 2999 dibujos, 343 contextos main y 626 de audio preservados.
- TitleScene original observado durante 97 esperas; el usuario vio parte del menú.
- Música completa sin problemas confirmada en esta corrida; cero bloques silenciosos con voces activas.
- Parada por 180 s; memoria gráfica final 28,1 MiB frente a pico 40,9 MiB y límite 64 MiB.
- Raster/hashes consumió 99,620 s; CPU sigue siendo el frente principal.

## Comprobar en la 86

| Área | Evidencia necesaria |
|---|---|
| Identidad | 86/01.90, iteration86-menu-boot-r1, Alice en LiveArea. |
| Preset | startup_intro_patch=applied; cuatro fingerprints; logo corto y transición a title. |
| Escena | Vtable 00658308 y update/draw originales; recursos/cierre adecuados sin opening. |
| Entrada | Scans pulsados/liberados corresponden a botones físicos y se refleja en selection/vertical. |
| Menú | Navegación, confirmación y cancelación reales; nueva escena/servicio o frontera identificada. |
| Tiempo | Menú permanece disponible tras 60 s de edad si no termina antes el diagnóstico. |
| Audio | Opening omitido con su música; comprobar efectos y audio de las acciones/escenas alcanzadas. |
| Rendimiento | Comparar tiempo hasta menú y costes; no dar por acelerado el renderer de CPU. |
| Cierre | Contextos, memory/refs, salida/watchdog válidos; time/draw/pixel scopes precisos. |

## Próximos pasos

1. Validar el preset y correlacionar los scans con la selección original.
2. Implementar la siguiente frontera que alcance la selección/personaje.
3. Diseñar backend GPU a partir del perfil D3D8 medido y referencias previas.
4. Completar controles/combate, colisiones, sonido, round y retorno al menú.
5. Verificar rendimiento/memoria/guardado sostenidos y precisión contra Windows.

96 grupos portables, fingerprints del original local, oráculos/replay,
VitaSDK y paquete comprobados. Salto e interacción físicos pendientes.
[Detalle, controles y paquete](STATUS_ITERATION86.md); [fuentes previas](RESEARCH_EXE_ITERATION73.md).
