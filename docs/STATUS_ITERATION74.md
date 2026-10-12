# Iteración 74 — EXE Message Loop / 01.78

## Evidencia física de la 73

El segundo quad ya ejecutó ONE/ZERO sobre el backbuffer X8R8G8B8 640×480:
307200 píxeles cubiertos y escritos, 219077 cambiados, cero rechazos por alfa.
Textura ABC010 de 1024², destino ABC000, diffuse FF0F0F0F y UV vertical
0..0.46875. Hash modulado 0315AC00; destino before 30E41DC5 y after 957A5E70.
Pasaron el epílogo COM de 24 bytes y EndScene en el retorno 40287C.

El EXE después abrió score.dat y leyó bloques de 91920, 8, 900, 40 y 256 bytes.
La siguiente frontera fue USER32.dll!PeekMessageA: IAT 65722C,
retorno 602F40, MSG 9FEE70, HWND NULL, filtros 0/0 y PM_NOREMOVE.
La API todavía no había ejecutado. Present no fue alcanzado: 0/8 frames.

9700 imports main, 2520 servicios D3D, seis DInput y 199 DirectSound.
30 slices, 15355524 us, sin límite/fault y watchdog desarmado.
El audio worker volvió a bloquearse tras un timeout real de 80 ms.
log.txt conserva las inicializaciones DirectGraphics/Input/Sound y LoadEffect OK.

## Cambio de la 74

- Atiende PeekMessageA con cinco parámetros y limpieza stdcall de 24 bytes,
  conservando las comprobaciones comunes de ESP/EIP y registros no volátiles.
- Asocia una cola de hasta 32 mensajes publicados al TIB y HWND propios.
  La cola se vincula después de los callbacks originales de creación;
  ShowWindow/UpdateWindow no borran mensajes ni cambian su propietario.
- Permite HWND NULL, propio o -1 para mensajes de thread; filtros inclusivos
  de 16 bits, PM_NOREMOVE, PM_REMOVE y PM_NOYIELD. Otros flags quedan como
  frontera explícita. WM_QUIT no se excluye por el filtro de rango.
- Una consulta sin mensaje devuelve FALSE y conserva MSG y LastError.
  Comprueba ambos después de la consulta. Los outputs aceptados se limitan
  a la pila guest RW; otros destinos necesitan un contrato de memoria separado.
- Una copia fallida no consume un mensaje. Un thread/ventana ajenos,
  callback pendiente o región invalidada detienen la ejecución para diagnosticar.
- Registra filtros, resultado, cantidad en cola, contador y ABI.
  Mantiene el límite anterior de ocho Present reales y el watchdog.
- Conserva el diagnóstico sólo EXE, X para salir, tres logs de iteración y
  compilación incremental. LiveArea conserva el icono original y badge 74/01.78.

En la ruta actual no hay productores de mensajes publicados: los callbacks de
ventana se ejecutaron sincrónicamente y la cola empieza vacía. El método de
publicación del modelo se comprueba en PC, pero aún no está conectado a
PostMessage, eventos del sistema ni controles de Vita. GetMessage/DispatchMessage,
timers, hooks y mensajes enviados adicionales siguen siendo trabajo pendiente.
La entrada DirectInput existente es un subsistema separado de esta cola.

La rama FALSE observada lleva a 602F76 y a la transición original 431F60;
esa rutina contiene una llamada a Present mediante 4028F0. Es una expectativa
del análisis estático: no acredita que la 74 ya llegue allí en hardware.
La espera posterior del main en 60353F también puede requerir más scheduler.

## Comprobaciones realizadas

Seis grupos nuevos de cola y seis de raster/presentación en PC, con patrones
propios y ASan/UBSan, todos aprobados. Véase
[la lista y sus límites](EXE_CHECKLIST_ITERATION74.md).
Compilación VitaSDK sin advertencias. ZIP CRC correcto, SFO 01.78/T075VITA1,
cuatro PNG indexados de ocho bits sin entrelazado. diff --check limpio.
Las pruebas PC no ejecutan el EXE ni validan la API de display nativa.
La nueva integración stdcall y Present necesitan ejecución física de la 74.

Build: iteration74-exe-message-loop-r1.
VPK local: artifacts/iteration74/Touhou75Vita-iteration74.vpk.
SHA256: b51a6c9eefebdd91f008ca8955da0f29d1b878ad76f12dd42c9b498cd79d3382.

## Siguiente ejecución en la Vita

1. Instalar el VPK 74/01.78; conservar EXE y archivos originales en TH075Vita.
2. Esperar al diagnóstico y fotografiar STOP y PRESENT FRAMES. X sale.
3. Enviar iteration74.log, iteration74-runtime.log, iteration74-watchdog.log,
   log.txt y captura.
4. Buscar startup_message_result=empty, output=preserved, LastError preserved
   y startup_service_stack_bytes=24. Después revisar present_request,
   present_native matches:yes, present=executed y el siguiente STOP/EIP.
5. Si muestra 8/8 y STOP Present, revisar present_scope_limit: el noveno
   Present se detiene deliberadamente. No demuestra menú ni combate jugable.

## Referencias usadas

La semántica de cola vacía, filtros y retirada proviene de
[Microsoft PeekMessageA](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-peekmessagea).
La cola propia y los callbacks siguen la separación descrita en
[Using Messages and Message Queues](https://learn.microsoft.com/en-us/windows/win32/winmsg/using-messages-and-message-queues).
El payload MSG público de 28 bytes para el guest PE32 se contrasta con
[la declaración Win32 de Wine](https://github.com/wine-mirror/wine/blob/master/include/winuser.h).
Implementación propia; no se copió código de esos proyectos.
Los descompiladores y proyectos externos permanecen documentados en
[RESEARCH_EXE_ITERATION73](RESEARCH_EXE_ITERATION73.md).
