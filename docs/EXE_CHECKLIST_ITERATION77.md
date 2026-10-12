# Comprobaciones para ejecutar TH075.exe en Vita

Estado físico tras la 76: dieciséis Present, dieciséis señales/esperas y
fade inicial terminado. La 77 intenta alcanzar la condición original del
logo (181 actualizaciones) y observar el cambio a la escena 0D. Hardware 77
pendiente; todavía no hay menú interactivo ni combate verificado.

## Orden de trabajo

| # | Comprobación | Criterio concreto | Estado |
|---|---|---|---|
| 1 | Identidad y carga PE32 | SHA del japonés 1.11, secciones/IAT y entrypoint correctos. | Confirmado en Vita. |
| 2 | ABI x86 y memoria | ESP/EIP, registros no volátiles, stdcall, TEB/FS/TLS, heap y accesos válidos. | Confirmado para los servicios usados; faltan casos futuros y ejecución sostenida. |
| 3 | Lectura de recursos original | Rutas, bytes leídos y offsets del DAT coinciden con lo solicitado por el EXE. | Carga inicial/texturas/effects alcanzada; falta cobertura completa. |
| 4 | Workers y sincronización | Threads bloquean/despiertan por eventos/timeout reales y permiten continuar el main. | 76 confirmó dieciséis SetEvent del worker 12, auto-reset waits consumidos y contextos preservados. 77 amplía la corrida; scheduler continuo de todos los workers pendiente. |
| 5 | Texturas y estados D3D8 | Formato, pitch, Lock/Unlock, refs, bindings y viewport válidos. | Confirmados para la ruta inicial. |
| 6 | Primer dibujo del EXE | 307200 cubiertos, 223567 escritos y hash de destino EF895E7C. | Confirmado físicamente en la 72. |
| 7 | Cierre del frame D3DX | EndScene, restore de targets, ApplyStateBlock y Releases sin corrupción. | Confirmado físicamente en la 72. |
| 8 | Segundo quad y fade | ONE/ZERO, diffuse FF0F0F0F, fuente 1024², UV vertical 0..0.46875 y backbuffer X8. | Confirmado físicamente en la 73: 307200 escritos, 219077 cambiados, hash final 957A5E70 y EndScene correcto. |
| 9 | Present original | RECTs/ventana correctos, frame real enviado a scanout, vblank y readback coincidentes. | 76 confirmó frames 1..16, RC 0 y matches:yes; hashes 1..8 iguales a la 75 y 11..16 estables. 77 reutiliza la conversión idéntica; set/vblank/query siguen ejecutándose en cada Present. |
| 10 | Bucle y entrada del menú | Procesar mensajes, reloj, entrada guest y aceptar una selección real. | 76 confirmó dieciséis vueltas y fade inicial terminado. 77 registra escena/vtable/edad del logo para observar el siguiente handoff. Entrada y selección real del menú pendientes. |
| 11 | Gráficos completos y memoria sostenida | Geometrías, filtros, mezcla, superficies y destrucción sin fugas; hashes/imágenes comparados con PC. | Renderer rectangular parcial; backend completo pendiente. |
| 12 | Combate, sonido guest y datos persistentes | Entrar a combate, controles, lógica/colisión, mixer, tiempos, round y regreso al menú. | Pendiente. El audio nativo del visor no acreditaba el mixer del EXE. |

Los PASS iniciales acreditan sus contratos. El juego será jugable cuando el
bucle original y las escenas cumplan estos criterios; un contador de imports
o una pantalla de recursos no proporciona un porcentaje de port completado.

## Comprobaciones ejecutadas para la 77

23 grupos portables propios con ASan/UBSan, -O2, -Wall -Wextra -Werror:

- Seis de raster/conversión y límites, seis de mensajes y seis de waits/
  contexto descritos en la [lista anterior](EXE_CHECKLIST_ITERATION76.md).
  Los límites compartidos ahora comprueban 240 frames/waits, 512 draws,
  128 Mi píxeles cubiertos y un cap de tiempo anterior al watchdog.
- Caché inicialmente vacía; snapshot confirmado; igualdad de copia con
  dirección distinta; scanout preparado idéntico al retenido.
- Cambios del primer byte, alpha, centro y último byte rechazan la caché.
- Dimensiones/pitch/formato/spans incorrectos, puntero nulo o que envuelve
  el espacio de direcciones se rechazan antes de leer.
- Fuente cambiada, slot válido, snapshot actualizado e invalidación explícita;
  un slot inválido deja la caché inutilizable.
- Solo la miniatura de un Present confirmado puede conservarse. El contador
  sigue aumentando; una fuente nueva regenera la miniatura.

Los seis grupos del renderer también aprobaron a -O0 con salida completa y
digests iguales a -O2. Los tests conservan asserts y usan patrones propios.
No ejecutan el EXE, no miden el SDK/display en Vita y no simulan fallos reales
de allocation/SetFrameBuf/vblank/query. Esas rutas mantienen guardas explícitas
y el próximo log físico debe confirmar su funcionamiento.

VitaSDK compiló el VPK con -O2 -g -DNDEBUG, -fno-fast-math y
-ffp-contract=off. ZIP CRC, SFO 01.81/T075VITA1 y los cuatro PNG indexados
sin entrelazado aprobaron. Véase [estado/SHA](STATUS_ITERATION77.md).

## Criterios de la siguiente corrida física

1. Build iteration77-exe-logo-handoff-r1 y pantalla 77 / versión 01.81.
2. Frames iniciales y fases iguales a la 76; señal real TIB 760000, contexto
   preservado y wait auto-reset consumido por cada continuación.
3. Tras estabilizarse el backbuffer, present_conversion=reused con igualdad
   de todos los bytes. Cada frame aún debe mostrar present_native con RC 0,
   matches:yes y present=executed. En un hit se conserva el slot sin escribirlo;
   ante un miss se prepara el otro buffer.
4. startup_frame_logo_age debe crecer por el código original. Observar edad
   181, estado 220D, fade de salida y cambio de vtable, o registrar la frontera
   exacta que impidió alcanzarlos. No inyectar teclas ni modificar ese contador.
5. Revisar present_elapsed_us y frame_cycle_elapsed_us para medir la caché.
6. Watchdog desarmado, sin corrupción/fault. 240/240 + STOP Present indica
   el cap deliberado antes del 241. Una parada de tiempo se identifica en
   startup_frame_wait_stop o startup_time_cap_us; no equivale a un menú logrado.

## Cómo acelerar las siguientes iteraciones

- Reproducir en PC los datos capturados de cada nueva frontera y comprobar
  píxeles/contratos antes de otra instalación en Vita.
- Preparar varios contratos cuyo ABI/semántica se conocen del binario;
  detenerse en los que siguen sin implementar. No retornar éxito vacío.
- Usar la captura Windows existente como referencia de llamadas/estados;
  capturar otra pasada con apitrace cuando haga falta comparar una escena.
- Análisis estático automático de las diez funciones del arranque/presentación
  listadas en [la investigación](RESEARCH_EXE_ITERATION73.md).
- La build 77 copia al árbol WSL sólo fuentes que cambiaron: conserva timestamps
  y permite compilación incremental. Eliminó includes y ejecución del visor DAT,
  del título nativo independiente y del reproductor BGM de diagnóstico.
- Mantener en Vita los archivos originales: el propio EXE seguirá leyéndolos.

La 77 permite hasta 240 Present/continuaciones reales dentro de 75 segundos,
con watchdog de 90 segundos y máximo de 64 slices. Continúa siendo una
captura acotada del arranque: la primera frontera desconocida detiene el EXE.
Los recursos de proyectos externos se evalúan según compatibilidad con TH075,
revisión y licencia; no se incorporó lógica de otro juego en esta iteración.
