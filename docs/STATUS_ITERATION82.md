# Iteración 82 — LINEAR Quads

## Evidencia física recibida de la 81

Se analizaron iteration81.log, runtime, watchdog, log.txt y la foto del usuario.
La confirmación más reciente es «escuché el audio por un instante».

- 203 Present reales: hashes de scanout y uploads de texturas iguales a 80.
- 204 waits del main atendidos y 204 contextos preservados.
- Play del buffer 00AE0138: 1 MiB, estéreo 44100 Hz/16 bits, flags looping 1.
- Salida nativa: 13 bloques de 1024 frames/48000 Hz = 0,2773 s enviados;
  26342 samples no cero, primer FNV 8DF03849, primer peak 54, fill máximo
  2931 us, output/drain/join/delete/release sin error.
- Worker 13 atendió ocho llamadas y volvió a bloquearse: cursores, evento,
  Lock, SetFilePointer, ReadFile de 131072 bytes, Unlock y GetStatus.
  El nuevo upload completo del stream tiene FNV FAA4890E. Los 40 uploads
  previos coinciden con 80. Esto prueba una reposición, no un stream continuo.
- Draw 230 POINT ejecutado; draw 231 detenido en DrawPrimitiveUP, retorno
  00402D44, textura ABCA48 A8R8G8B8 de 512×512 con un nivel. Filtros
  MAG/MIN/MIP = 2/2/2, WRAP U/V, strip de dos primitivas, FVF 144, stride 28.
  Rectángulo registrado aproximadamente (-128,495,-116,5)–(383,505,395,5),
  UV 0..1, diffuse FFFFFFFF. El log redondea coordenadas al imprimirlas.
- No hubo Present 204: la captura conserva el negro del frame 203.
  Gráficos 42896712 bytes de 67108864; tiempo 70,338 s, sin límite CPU,
  watchdog desarmado. Arranque completo, menú y combate no verificados.

El sonido breve es coherente con la poca duración enviada antes de la
frontera gráfica y el cierre explícito de la salida al terminar el diagnóstico.
No se reproduce una pista alternativa para prolongarlo.

## Cambios de la 82

1. Filtrado bilineal propio en el renderer rectangular: coordenada u*N−0,5,
   cuatro vecinos con WRAP individual, pesos por distancia y redondeo final
   de canales a ocho bits. Se interpola RGBA sin premultiplicar antes de
   MODULATE, GREATEREQUAL y mezcla SRCALPHA/INVSRCALPHA o ONE/ZERO.
   A8R8G8B8/A1R5G5B5 como fuente y A8R8G8B8/X8R8G8B8 como destino.
2. El perfil COM admite min/mag iguales POINT o LINEAR, MIP NONE/POINT/LINEAR
   sobre el único nivel que permite CreateTexture. No se anuncian capacidades
   generales ni se simula LINEAR usando POINT. Filtros mixtos/anisotrópicos,
   múltiples niveles, geometrías y estados fuera del perfil no escriben destino.
3. Logs draw_filter incluyen modo, dimensiones, un nivel y significado del
   probe: vecino superior izquierdo envuelto y color filtrado. Presupuestos
   anteriores conservados; no se añaden assets al VPK.
4. La estimación de cursor PCM conserva la fase absoluta máxima observada
   dentro del epoch. La 81 había registrado 11052→7524 bytes cuando creció
   la cola nativa antes del commit de Output. El ancla evita ese retroceso;
   se aplica módulo después, y seek/Stop/completion reinician el ancla.
   Sigue siendo una estimación de bloques: precisión, latencia y cambios de
   frecuencia en reproducción requieren medición adicional en hardware.

## Referencias y límites de fidelidad

La documentación de Microsoft describe interpolación de cuatro texels y
sus centros normalizados. Se usa como referencia de las mismas operaciones
de muestreo del perfil D3D8 observado, aunque la página es de Direct3D 9.
La precisión interna de una GPU D3D8 puede cambiar un canal en una unidad;
no se afirma identidad con Windows sin una captura comparable.
[Microsoft: bilinear](https://learn.microsoft.com/en-us/windows/win32/direct3d9/bilinear-texture-filtering).
POINT=1 y LINEAR=2 tienen usos distintos para min/mag y MIP; con un único
nivel, MIP no puede seleccionar otra imagen.
[Microsoft: filtros](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dtexturefiltertype).

Se mantienen WinVita/Box86, lector TH075 propio, los contratos de VitaSDK y
las referencias de N0zoM1z0 y otros ports documentadas en
[investigación](RESEARCH_EXE_ITERATION73.md). La implementación es propia;
no se copió código de Wine ni descompilación del juego al repositorio.

## Comprobaciones ejecutadas

64 grupos portables a -O2, -Werror y ASan/UBSan: 54 regresiones previas,
ocho del filtro LINEAR, una del DrawPrimitiveUP real del puente y una
del cursor con crecimiento de cola. Incluyen:

- Centros de texels, cuatro pesos, redondeo, costuras WRAP y UV negativos.
- Alpha interpolada, modulación, umbral, blend, A1R5G5B5 y textura de un texel.
- Geometría fraccional representativa de 81, comparada píxel por píxel contra
  un cálculo independiente con textura sintética; 152064 píxeles cubiertos.
- Pitch con padding, viewport parcial, X8, filtros inválidos y alias sin escritura.
- DrawPrimitiveUP del código enviado a Vita: LINEAR 2/2/2, stdcall,
  registros no volátiles, HRESULT y rechazo de filtros mixtos/anisotrópicos.
- Fase de audio frente a crecimiento de cola, wrap legítimo y seek/Stop.

Replay local de 1312 llamadas D3D de ownership/estado, sin ejecutar x86.
Resultados/digests POINT y LINEAR idénticos a -O0/-O2. El digest LINEAR de
la textura sintética es 0EC8D2DC; no es un hash esperado del DAT real.
Los tests usan memoria/samples sintéticos y APIs nativas simuladas: no
ejecutan EXE, DAT, ARM ni una consola.

VitaSDK compiló con -O2, símbolos y FP estricto. ZIP CRC, SFO
01.86/T075VITA1 y cuatro PNG indexados de ocho bits aprobados.
Build: iteration82-linear-quads-r1.
VPK: artifacts/iteration82/Touhou75Vita-iteration82.vpk.
SHA256: d5341ea713efb4e8d5b1925a002f2dcaefb9f8260230baf41c4882bea288cbd5.
LiveArea 82/01.86 editado con image_gen integrado; prompt y rutas en
artifacts/iteration82/livearea-generation.txt, fuera de Git.

## Siguiente corrida física

1. Instalar 82/01.86, conservar EXE/DAT originales; X sale.
2. Esperar hasta 100 s al diagnóstico (watchdog 120 s).
3. Enviar iteration82.log, runtime, watchdog, log.txt y foto. Indicar duración
   aproximada del audio y si se interrumpe o queda fluido.
4. Confirmar 203 hashes previos, buffers/uploads y ausencia de nuevos errores.
5. Revisar draw_filter 2/2/2, perfil y hashes posteriores al draw 230;
   geometrías posteriores pueden seguir requiriendo implementación.
6. Comprobar si hay Present 204 en adelante y si aparece una escena visible.
7. Revisar audio_worker_dispatches, refills, cursores/monotonic_phase, output
   y costo; distinguir wrap del buffer de un retroceso del cursor.
8. Registrar próxima frontera, contexto, memoria, tiempo y cierre de recursos.

Hardware 82 está pendiente. No se garantiza alcanzar 240 frames ni entrar
al menú. Continuidad del audio y demo jugable siguen pendientes.
Véanse [checklist](EXE_CHECKLIST_ITERATION82.md) y [roadmap](ROADMAP.md).
