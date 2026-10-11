# Iteración 73 — EXE Present / 01.77

## Evidencia física de la 72

Primer quad ejecutado: 307200 píxeles cubiertos, 83633 rechazados por alfa,
223567 escritos/cambiados. Hash source 6948C5D1, before 07641DC5, after EF895E7C.
La pantalla mostró el logo extraído por la ejecución original del EXE.
Pasaron EndScene, restore del backbuffer/profundidad, ApplyStateBlock y Releases.

La nueva frontera fue un segundo DrawPrimitiveUP: textura ABC010 de 1024²,
destino ABC000 X8R8G8B8 640×480, diffuse FF0F0F0F, U 0..0.625 y V 0..0.46875.
Los factores cambiaron a ONE/ZERO (2/1); el guard de la 72 sólo aceptaba
SRCALPHA/INVSRCALPHA y detuvo correctamente ese draw.

9690 llamadas main, 2518 servicios D3D, seis DInput, 199 DirectSound,
30 slices y 19684982 us. EIP BFC480, sin límite ni CPU fault; watchdog disarmed.
log.txt conserva LoadEffect ...OK. Present no fue alcanzado en la 72.

## Cambios

- Retira de la build y de la pantalla el navegador/exportador DAT, el reproductor
  BGM auxiliar y la lectura independiente de title_preview. No genera bgm.log.
  La pantalla usa únicamente una captura originada en el renderer del EXE y X
  para salir. El EXE conserva sus archivos originales y contratos necesarios.
- Añade ONE/ZERO con ADD al perfil rectangular: modula los cuatro canales
  antes de alpha test y copia el color resultante; X8 normaliza X=FF.
  La referencia de factores es
  [D3DBLEND](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dblend).
- Implementa Present para el cliente completo 640×480, sin dirty region y
  ventana NULL o propia. Valida los RECTs guest y el backbuffer antes de escribir.
  Otros rectángulos/ventanas/escenas abiertas conservan una frontera explícita.
- Present copia el backbuffer a dos buffers CDRAM alternados, convierte ARGB a
  ABGR y conserva proporción con imagen 725×544 y bandas negras. Comprueba
  SetFrameBuf NEXTFRAME, vblank y que el framebuffer activo coincida.
  HRESULT success y epílogo COM sólo se producen tras esa confirmación.
- Límite de ocho Present reales por ejecución. Registra RECTs, RC nativos,
  hash del scanout, frame y ABI. El diagnóstico conserva el último backbuffer
  presentado; antes del primer Present conserva el primer quad.
- Libera buffers tras comprobar/detener scanout; si no puede confirmar,
  difiere su liberación a la salida del proceso.
- Script local WSL copia sólo archivos diferentes, evitando recompilar fuentes
  por actualizaciones innecesarias de timestamp.

El límite permite detectar varias fronteras o frames en un mismo VPK.
No garantiza llegar a Present: otro servicio del original puede detenerse antes.
No hay menú ni combate verificados; no es todavía una sesión jugable continua.

## Validación

Seis grupos de comprobaciones PC aprobados con ASan/UBSan, descritos en
[el listado de pruebas](EXE_CHECKLIST_ITERATION73.md). Usan patrones propios.
Compilación VitaSDK sin advertencias; ZIP CRC correcto, SFO 01.77/T075VITA1 y
cuatro PNG indexados de 8 bits, sin entrelazado. LiveArea conserva el icono del
EXE e identifica iteración 73/version 01.77. diff --check limpio.
Nueva ejecución física pendiente; Present nativo y ABI necesitan los logs 73.

Build: iteration73-exe-present-r1.
VPK: artifacts/iteration73/Touhou75Vita-iteration73.vpk.
SHA256: 9ec3efdaa6f0c53452b9ac632cf8ce2a0400f55248283509003abc4911205eb3.

Enviar iteration73.log, iteration73-runtime.log, iteration73-watchdog.log,
log.txt y captura. Revisar draw_blend=one_zero, segundo draw, present_request,
present_native matches:yes, present=executed, cleanup=24 y siguiente STOP/EIP.
Si muestra PRESENT FRAMES 8/8 y STOP Present, comprobar present_scope_limit:
sería el límite deliberado de esta fase.

La [investigación de descompilación y recursos](RESEARCH_EXE_ITERATION73.md)
incluye Ghidra/RetDec, uso concreto de los proyectos previos y diez funciones
seleccionadas. El exportador Ghidra está preparado; aún falta ejecutarlo con
una instalación compatible. No se afirma una nueva descompilación del binario.
