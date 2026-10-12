# Iteración 75 — EXE Frame Events / 01.79

## Evidencia física de la 74

PeekMessageA devolvió cola vacía, preservó MSG/LastError y limpió 24 bytes
en el retorno 602F40. El EXE continuó por su propia rama de frame.
Su Present en el retorno 40291E completó la limpieza COM de 24 bytes
y devolvió HRESULT 0. RECTs de origen/destino: 0,0,640,480; ventana y dirty
region NULL; EndScene ya completo.

La API nativa set/vblank/query devolvió RC 0, el framebuffer activo coincidió
(matches:yes) y se presentó el backbuffer ABC000. Frame 1 de 8;
scanout FNV B047B6F7. Fuente 640×480, salida 960×544, imagen centrada 725×544.
La captura física muestra el logo oscuro correspondiente al fade original.
Los dos buffers de display se liberaron con RC 0 al finalizar el diagnóstico.

La nueva frontera fue KERNEL32.dll!WaitForSingleObject: IAT 657120,
trap B003C0, retorno 603545, ESP 9FEC48 y EBP 9FEED0.
Evento AB2004, auto-reset sin señal, timeout FFFFFFFF (INFINITE).
La espera no se ejecutó. No fue un límite de CPU: 30 slices, 18521680 us,
sin fault/límite y watchdog desarmado. 9740 imports main, 2557 D3D,
seis DInput y 199 DirectSound. El worker de audio volvió a bloquearse
tras un timeout real de 80 ms; log.txt conserva LoadEffect OK.

## Productor encontrado en el EXE

El arranque crea el evento de frame en 602CFB y registra el puntero a su
handle mediante 423C80. La lista original se encuentra en 68BE44.
El worker 12, entrada 423B40, espera sobre AB2000 con periodo de 16 ms
(global 66C23C). Tras el timeout recorre la lista y llama SetEvent,
IAT 65705C, retorno 423BF5. Main espera AB2004 en 60353F y vuelve a
602F21 después del retorno 603545.

Ese recorrido se obtuvo del análisis estático del binario local y de los
handles/waits capturados. La señal real del worker para AB2004 todavía
necesita confirmación física en la 75.

## Cambios de la 75

- Conserva intacta la llamada pendiente del main al reconocer exclusivamente
  el TIB/pila/retorno/IAT/evento/INFINITE de la frontera física de la 74.
- Reanuda el worker original después de su timeout real de 16 ms. Comprueba
  el periodo global, handle y frame de wait guardado; no inventa una señal.
- Requiere un SetEvent real del worker y cambio de generación del evento.
  Reintenta el contrato de WaitForSingleObject para consumir el auto-reset.
- Comprueba WAIT_OBJECT_0, ESP +12, retorno EIP, evento consumido y LastError.
- Guarda explícitamente el TIB de cada contexto: Cpu::save_context delega
  FS al scheduler. Valida GPR/EIP/flags/TIB/FPU al restaurar main.
- Inicializa el padding del snapshot FPU y sus bytes no usados en WinVita,
  evitando comparaciones inestables de relleno. Conserva los campos x87/MMX/SSE
  y la semántica previa de la primera ejecución de un worker.
- Si el worker no señala o llega a otro servicio no implementado, conserva
  la frontera para diagnosticar. No devuelve éxito a waits arbitrarios.
- Permite ocho continuaciones de frame como máximo. El cap de slices aumenta
  de 32 en cuatro por evento consumido, hasta 64; mantiene la comprobación de
  progreso, cap de 45 s y watchdog de 60 s. El noveno Present se detiene.
- Diagnóstico sólo EXE, X para salir; logs propios de la 75, build incremental
  y LiveArea con identificación ITERACION 75 / VERSION 01.79.

No hay scheduler continuo de todos los workers ni audio guest funcional
confirmado. Ocho frames sólo cubrirían una parte del fade; no acreditan el
menú ni combate. Cada nueva frontera debe conservar su contrato verificable.

## Comprobaciones y paquete

18 grupos portables propios aprobados con ASan/UBSan:
seis de wait/contexto, seis de mensajes y seis de raster/presentación.
Véase [lista y límites](EXE_CHECKLIST_ITERATION75.md).
La compilación VitaSDK fue exitosa. Al recompilar cpu_box86.cpp aparecieron
dos advertencias existentes de variables extern inicializadas; las unidades
del proyecto Vita compilaron sin nuevas advertencias.
ZIP CRC correcto; SFO 01.79/T075VITA1; cuatro PNG indexados de ocho bits,
sin entrelazado. La integración de la 75 sigue pendiente en hardware.

Build: iteration75-exe-frame-events-r1.
VPK local: artifacts/iteration75/Touhou75Vita-iteration75.vpk.
SHA256: e6f5f1454dd03085e4468596a439c7033340b262f1d4a8e85bff038d0b620331.

## Siguiente ejecución en la Vita

1. Instalar el VPK 75/01.79, conservando el EXE y sus datos originales.
2. Esperar al diagnóstico; capturar STOP y PRESENT FRAMES. X sale.
3. Enviar iteration75.log, iteration75-runtime.log, iteration75-watchdog.log,
   log.txt y captura.
4. Revisar contexto conservado, señal real del timer, wait consumido y
   contador startup_frame_wait_resumes, además de los Present ejecutados.
5. Si aparece 8/8 y STOP Present, revisar present_scope_limit: el noveno
   Present es una frontera deliberada de esta comprobación.

## Referencias y reutilización

La espera INFINITE y WAIT_OBJECT_0 siguen
[Microsoft WaitForSingleObject](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject).
La señal y el consumo auto-reset siguen
[Microsoft SetEvent](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-setevent).
El handoff usa el runtime WinVita/Box86 ya integrado; el contrato acotado
del arranque es propio. Las referencias de recompilación, N0zoM1z0 y
descompiladores siguen en [RESEARCH_EXE_ITERATION73](RESEARCH_EXE_ITERATION73.md).
Los recursos, el EXE, el arte LiveArea, logs completos y decompilaciones
permanecen locales fuera de Git.
