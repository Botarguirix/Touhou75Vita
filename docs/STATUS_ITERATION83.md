# Iteración 83 — Audio Scheduling

## Evidencia física de la 82

El usuario confirmó la instalación y corrigió que el error anterior provenía
de la consola. No se sustituyó el paquete 82 por una variante de instalación.
Se archivaron sus tres logs, log.txt y foto fuera de Git.

- 208 Present reales, 209 contextos main preservados. Los primeros 203 hashes
  de scanout coinciden con 81. Se ejecutaron 397 dibujos, 167 con LINEAR.
- El raster y sus hashes consumieron 68,308 s. Los dibujos LINEAR de pantalla
  completa más lentos rondaron 786 ms. El último ciclo tardó 4,338 s.
- Tiempo total 104,245 s: el límite de 100 s se revisó al terminar el ciclo.
  Sin límite CPU, watchdog desarmado, parada en WaitForSingleObject del main.
  La captura final es negra; entrada al menú y combate siguen pendientes.
- Gráficos: 42896712 bytes dentro de 64 MiB, sin error de salida o liberación.
- Un solo Play del stream 00AE0138, estéreo 44100 Hz/16 bits y anillo de 1 MiB.
  1191 bloques de 1024 frames a 48 kHz = 25,408 s enviados a la salida;
  1297405 samples no cero, primer FNV 8DF03849, output/drain RC 0.
- Seis despachos del worker 13 durante el bucle de frames. Hizo dos lecturas
  de reposición de 131072 bytes, con uploads completos FAA4890E y 039F1875.
  Los otros cuatro despachos consultaron cursor/status y volvieron a esperar.
  Tras Play, sus esperas reales fueron 3,852–4,288 s frente a 80 ms pedidos.
- Audibilidad confirmada: el usuario escuchó tramos de unos 3–4 s repetidos
  aproximadamente 3–4 veces. Enviar 25,408 s no demuestra continuidad audible.

Los retrasos de servicio y el anillo looping son compatibles con la repetición
de datos antiguos. No hubo nuevas llamadas Play que expliquen los reinicios.
Esto es una hipótesis sustentada por logs; no una prueba de causa única.

## Cambios de la 83

1. Despacho cooperativo del worker original 13 antes de llamadas main a
   imports/COM conocidos, cuando su timeout real de 80 ms haya vencido y haya
   reproducción activa. El callback devuelve el control al runner; el worker
   se ejecuta después de que Cpu::run termine, con el mismo emulador.
2. Se preservan GPR, flags, FS/TEB, x87/SIMD, LastError y doce bytes del callframe.
   Se reintenta la llamada original sin consumirla ni duplicar su servicio.
   No se despacha con una sección crítica guest adquirida, un callback de
   ventana en vuelo o un Lock PCM pendiente. Una frontera del worker se conserva.
3. Logs audio_trap_yield/context/dispatches registran preservación y retraso.
   Máximo 1024 despachos entre llamadas, límite de tiempo y watchdog existentes.
   No hay preempción dentro de un dibujo nativo o un tramo x86 sin imports;
   un servicio largo todavía puede retrasar audio. No es un scheduler completo.
4. LINEAR usa enteros sólo cuando ambos pesos son exactamente múltiplos de
   1/65536. No cuantiza ni redondea coeficientes: los significandos intermedios
   caben en la precisión del cálculo double anterior, con el mismo redondeo
   final por canal. Centros exactos usan un texel y vecinos idénticos evitan
   interpolación; otros pesos conservan el cálculo double. MODULATE, alpha,
   blend, WRAP, validaciones y hashes siguen iguales. Los contadores de cada
   camino permiten medir qué se utilizó. Aceleración en Vita pendiente.
5. El opening observado usa 28 dibujos por frame. Para que el límite previo
   de 512 dibujos/128 Mi píxeles no interrumpa antes de 240 Present, se amplía
   el presupuesto a 2048 dibujos/384 Mi píxeles cubiertos. Memoria gráfica
   permanece en 64 MiB; 240 frames/waits, 100 s, watchdog 120 s y 64 slices.
6. LiveArea, fondo de arranque, icono y gate se construyen desde el frame 6
   original de data/system/selectchar.dat (Alice), leído nuevamente de th075.dat.
   La herramienta prepare_dat_livearea.py conserva la procedencia y los hashes
   fuera de Git. Cada PNG parte del RGBA original y se codifica una vez al
   perfil indexado de ocho bits; no se vuelve a editar una portada previa.

La música proviene exclusivamente del PCM cargado por el EXE. La portada no
es una escena del juego ejecutado. No se restableció el navegador DAT ni se
añadieron pistas de diagnóstico. Los DAT completos y el EXE siguen externos.

## Comprobaciones

66 grupos portables a -O2, -Werror y ASan/UBSan, incluyendo 200000 muestras
bilineales con pesos exactos comparadas contra el cálculo double anterior,
coeficientes no exactos, vecinos uniformes, gating del timeout y secciones
críticas. Digests POINT/LINEAR sin cambios e idénticos a -O0/-O2.
Replay de 1312 llamadas de ownership D3D conservado.

Los checks de scheduler validan la política portable; no ejecutan este runner
x86/ARM ni prueban la continuidad física. La preservación en el runner nuevo
se registra para su próxima corrida. VitaSDK compiló el VPK con -O2 y FP estricto.

Build: iteration83-audio-scheduling-r1. Versión 01.87, T075VITA1.
VPK local: artifacts/iteration83/Touhou75Vita-iteration83.vpk.
SHA256: be33adb09c5309aa7e2038d5a742c5cb8b394b413c616add1eb9aa73f6c2640a.

## Próxima corrida física

Instalar 83/01.87 y comprobar la portada de Alice y pantalla ITERATION 83.
Conservar los DAT/EXE y esperar al resultado (X sale). Enviar iteration83.log,
iteration83-runtime.log, iteration83-watchdog.log, log.txt y foto. Registrar
duración aproximada de la música, repetición/silencios y escena visible.

Revisar tiempos de draw/frame, contadores LINEAR, los 203 hashes anteriores,
Present posteriores, audio_trap_context=preserved, tiempos del worker, lecturas
originales y cursores. Terminar a 240 frames sigue siendo diagnóstico acotado;
no demuestra arranque completo, un menú interactivo ni un juego jugable.

Referencias de contratos/decompiladores y proyectos previamente revisados:
[investigación](RESEARCH_EXE_ITERATION73.md),
[LINEAR](STATUS_ITERATION82.md), [checklist](EXE_CHECKLIST_ITERATION83.md).

## Resultado físico de la 83 (10 de octubre de 2026)

228 Present reales, 695 dibujos (465 LINEAR), 105233024 píxeles cubiertos,
230 contextos de espera main y 189 contextos de despacho audio preservados.
Los primeros 208 hashes scanout coinciden con 82; también los hashes de sus
primeros 397 dibujos. Esos mismos dibujos consumieron 43,172 s frente a
68,308 s en 82. Es una medición de estas corridas, no una tasa garantizada.
El total del raster/hashes de 83 fue 60,435 s y la ejecución 99,672 s.

Paró antes de DrawPrimitiveUP número 696, textura 00ABCAB8: sus cuatro vértices
forman un strip ligeramente inclinado. La ruta rectangular lo rechazó sin
escribir. La escena final ya muestra una imagen marrón del opening con texto
japonés, no la captura negra anterior. No hay menú o combate confirmado.

Un Play, 44 uploads PCM activos, 1613 bloques aceptados (34,411 s enviados),
output/drain/join/delete/release RC 0. El usuario confirma que duró algo más,
pero seguía entrecortada, y observó uso de CPU al máximo. Tras el primer wake,
los despachos de audio tuvieron mediana 138064 us y máximo 508982 us frente
a 80000 us solicitados. El primer wake incluye 55,094 s de inicialización
previa a Play: no equivale a una pausa audible de 55 s.

El anillo mide 1 MiB, aproximadamente 5,94 s a 44100 Hz estéreo/16 bits;
estos retrasos por sí solos no demuestran que faltasen datos de música.
No se registraron intervalos de salida nativa en 83: continuidad y causa de
los cortes siguen pendientes. El 1 FPS de la foto de resultados no mide
por sí solo el FPS del EXE. Watchdog desarmado y memoria gráfica 42896712
de 67108864 bytes. Logs/foto completos archivados fuera de Git.
