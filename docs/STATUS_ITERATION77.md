# Iteración 77 — EXE Logo Handoff / 01.81

## Evidencia física de la 76

Los logs recibidos confirman 16 Present reales con set/vblank/query RC 0 y
matches:yes. El worker original 12, TIB 760000, produjo 16 señales sobre
AB2004; main consumió 16 waits auto-reset con retorno WAIT_OBJECT_0,
limpieza de 12 bytes y GPR/flags/FS/FPU/callframe/LastError preservados.

| Frames | Scanout FNV-1a / estado |
|---|---|
| 1..8 | Idénticos a la 75: B047B6F7, 44B6FA68, 0BA47480, AF928E16, 686641A5, 5BC8CC09, E35328A7, 1CF81F90. |
| 9 | B81561B2 |
| 10 | 94C11727 |
| 11..16 | 762F684B; logo completamente visible. |

La palabra de estado original pasó de 2004 a 0004 tras el frame 11;
66C240 pasó de 9..0 a FFFFFFFF. Esto acredita el fade inicial terminado.
La parada fue el cap configurado antes del Present 17: EIP BFC0F0,
ESP 9FEC24, EBP 9FEC40. 30 slices, 2999 servicios D3D, seis DInput,
199 DirectSound y 10246 imports del main. Sin fault ni límite de CPU;
watchdog desarmado. log.txt conserva inicializaciones y LoadEffect OK.

La corrida tardó 21210139 us (21,21 s); la 75 tardó 37,37 s con un cap de
ocho frames. Los alcances son diferentes: esos totales no son un benchmark
controlado ni prueban un factor de aceleración específico.
Los seis últimos draws de logo duraron 140913..141230 us. Present, incluida
conversión y miniatura, duró 306021..322423 us (mediana 310908 us).
Los cinco ciclos completos de logo estable duraron 484123..503458 us.
La conversión repetida es un coste medido que esta iteración intenta reducir.

## Lo que falta para cambiar de escena

Análisis estático del EXE japonés con SHA verificado:

- 425490 construye el logo con vtable 657BE0; update 4255D0 y draw 425620.
- update dibuja y presenta, luego incrementa el WORD original en objeto+0C.
  Mientras edad <=180 devuelve 0004; al llegar a 181 devuelve 220D.
- main, en 602FAE, llama ese update. 431F60 procesa el fade; main conserva
  la escena mientras existe la fase 0F00 y luego construye la siguiente.
- El índice bajo 0D usa la tabla de 60360B: rama 603498, constructor 4275D0.

La 76 solo completó cinco updates del logo después del fade (frames 12..16).
Una corrida de 32 frames seguiría sin alcanzar la condición de edad 181.
La condición y la siguiente rama están identificadas; su ejecución física,
las próximas texturas y el menú aún no están confirmados.

## Implementación de la 77

1. Hasta 240 Present y 240 continuaciones reales, con cap de 75 s,
   watchdog de 90 s y máximo de 64 slices. Las mediciones indican que conservar
   el antiguo cap de 45 s podría detener el logo antes de la condición original.
2. Caché opcional de 1228800 bytes del último backbuffer confirmado. Solo
   acepta 640x480, X8R8G8B8, pitch 2560 y span exacto. Compara todos los
   bytes con memcmp; ningún hash decide si dos imágenes son iguales.
3. En igualdad conserva el scanout confirmado sin escribir el buffer activo.
   Si cambian los bytes, prepara el buffer inactivo. Cada Present sigue
   llamando SetFrameBuf, esperando vblank y verificando el framebuffer activo.
   Un fallo nativo invalida la caché; un fallo de snapshot vuelve a conversión
   normal. Solo se guarda después de confirmación nativa exitosa.
4. Conserva la miniatura únicamente si corresponde al Present anterior
   confirmado e idéntico; actualiza el contador de frames en cada Present.
5. Cada draw, wait y actualización del EXE continúa ejecutándose. Lee escena
   [EBP-184], vtable, update/draw y edad del logo en la espera conocida;
   valida stack/spans antes de leer y no modifica ningún contador/estado.
6. Límites de dibujo: 512 calls y 128 Mi píxeles cubiertos durante la corrida.
   Es un presupuesto de trabajo acumulado, no una reserva de 128 MiB.
   La asignación de recursos D3D permanece acotada a 32 MiB.

El cambio reduce trabajo nativo repetido conservando el avance original
del juego. La ganancia de tiempo y el comportamiento del scanout reutilizado
deben medirse en la Vita; el paquete aún no acredita un menú jugable.
El diagnóstico mantiene X para salir y LiveArea 77 / 01.81.

## Comprobaciones y paquete

23 grupos portables propios aprobaron con ASan/UBSan: 18 anteriores y cinco
de caché/miniatura. El renderer a -O0 y -O2 produjo salidas y digests idénticos.
Véanse [casos concretos y límites](EXE_CHECKLIST_ITERATION77.md).
VitaSDK compiló exitosamente, sin advertencias en esta recompilación.
ZIP CRC correcto, SFO 01.81/T075VITA1, cuatro PNG indexados de ocho bits y
sin entrelazado. Build -O2 -g -DNDEBUG, FP estricto. Hardware 77 pendiente.

Build: iteration77-exe-logo-handoff-r1.
VPK local: artifacts/iteration77/Touhou75Vita-iteration77.vpk.
SHA256: a538e8a56e15abd396654bdd7a15c47da269a299ce04ada8df54b24134fca355.

## Próxima comprobación en Vita

1. Instalar 77/01.81 conservando TH075.exe y sus archivos de datos originales.
2. Esperar al diagnóstico (cap 75 s; watchdog 90 s). Fotografiar STOP,
   PRESENT FRAMES y la captura del EXE. X sale del diagnóstico.
3. Enviar iteration77.log, iteration77-runtime.log, iteration77-watchdog.log,
   log.txt y captura.
4. Revisar hashes iniciales, caché reutilizada con RC/matches válidos, edad,
   fases, cambio de vtable, tiempo y frontera final. Si hay un servicio/estado
   nuevo, sus argumentos definirán la siguiente implementación.

## Roadmap y referencias

Estamos en el bucle original de logo y presentación. El próximo hito es
cambiar de escena con la lógica del EXE; después faltan menú, controles,
scheduler continuo del audio, mixer original y combate. El visor de música
y DAT sigue eliminado del VPK. No se incorporó código de otro título.
La arquitectura y recursos externos siguen en
[la investigación](RESEARCH_EXE_ITERATION73.md) y el estado global en
[ROADMAP](ROADMAP.md).
Las operaciones nativas conservadas corresponden a
[SceDisplay de VitaSDK](https://docs.vitasdk.org/group__SceDisplayUser.html):
SetFrameBuf, WaitVblankStart y GetFrameBuf.
La inferencia de 181 updates y escena 0D procede del binario local original,
no de código reconstruido de otro juego.
